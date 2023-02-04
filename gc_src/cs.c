#include "cs.h"
#include "mb.h"
#include "ms.h"
#include "virt.h"
#include "../util_src/data.h"
#include "../core_src/mem.h"
#include "../core_src/thread.h"
#include "../core_src/sys.h"
#include "../core_src/io.h"

#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>

// NOTE: Rigorous GC Notes:
// 
// Garbage collection will be an algorithm with a start
// and finish which frees memory it guarantees is unreachable by the user.
// 
// I will attempt to keep GC completely concurrent. That is, it will never stop
// the entire process. Instead it will pop in and out of user objects using
// a background thread.
//
// There will be one root object which is always reachable. (Or "Live")
// An arbitrary user object o is "Live" if there exists a path of references
// starting at the root and ending at o. 
//
// GC will start with a "paint white" phase. By the end of this phase, all objects
// will be marked "unvisited" or "newly added".
//
// "newly added" will refer to objects which were created too late for
// the algortihm to consider them for garbage collection.
//
// GC will continue with a "paint black" phase. This phase will strategically
// "visit" user objects.
// 
// This phase ends when it can guarantee :
//      1) All reachable objects are marked "visited" or "newly added".
//      2) All objects which were unreachable before the "paint white" phase
//         started are marked "unvisited".
//
// After this phase, a simple "sweep" phase will occur, which will free all objects
// marked "unvisited".
//
//
//

typedef enum {
    GC_NEWLY_ADDED = 0,
    GC_UNVISITED,
    GC_VISITED,
    GC_ROOT,
} gc_status_code;

typedef struct {
    gc_status_code gc_status;
} obj_pre_header;

static const uint64_t GC_STAT_STRINGS_LEN = 4;

static const char *GC_STAT_STRINGS[GC_STAT_STRINGS_LEN] = {
    "GC Newly Added",
    "GC Unvisited",
    "GC Visited",
    "GC Root", 
};

typedef struct {
    uint8_t allocated;

    union {
        cs_root_id next_free;
        addr_book_vaddr vaddr;
    };
} root_set_entry;

struct collected_space_struct {
    mem_space * const ms;

    // NOTE: the gc_space with have a set of "root objects".
    // A root object is just like any other object.
    // It will be stored in the memory space and will
    // be refered to by a virtual address.
    //
    // NOTE: This lock must be held whenever the root status
    // of an object is being edited... not just for when
    // we are editing the set below.
    pthread_rwlock_t root_set_lock;

    uint64_t root_set_cap;

    // If this is UINT64T_MAX, the free list is empty.
    cs_root_id free_head;
    root_set_entry *root_set;
};

// TODO: rewrite the collected space code here!

collected_space *new_collected_space_seed(uint64_t chnl, uint64_t seed, 
        uint64_t adb_t_cap, uint64_t mb_m_bytes) {
    collected_space *cs = safe_malloc(chnl, sizeof(collected_space));

    // Create our underlying memory space.
    *(mem_space **)&(cs->ms) = 
        new_mem_space_seed(chnl, seed, adb_t_cap, mb_m_bytes);

    // Set up our root set.
    // Starts with one single free entry.
    safe_rwlock_init(&(cs->root_set_lock), NULL);
    cs->root_set = safe_malloc(chnl, sizeof(root_set_entry) * 1);
    cs->root_set_cap = 1;

    uint64_t free_head = 0;

    cs->root_set[0].allocated = 0;
    cs->root_set[0].next_free = UINT64_MAX;

    return cs;
}

void delete_collected_space(collected_space *cs) {
    safe_rwlock_destroy(&(cs->root_set_lock));
    safe_free(cs->root_set);

    delete_mem_space(cs->ms);
    
    safe_free(cs);
}

static malloc_res cs_malloc_p(collected_space *cs, uint64_t rt_len,
        uint64_t da_size, uint8_t root, uint8_t hold) {
    malloc_res res = ms_malloc_and_hold(cs->ms, 
            sizeof(obj_pre_header) +
            sizeof(obj_header) +
            (sizeof(addr_book_vaddr) * rt_len) +
            (sizeof(uint8_t) * da_size));

    // Set up headers...
    obj_pre_header *obj_p_h = res.paddr;
    obj_header *obj_h = (obj_header *)(obj_p_h + 1);
    addr_book_vaddr *rt = (addr_book_vaddr *)(obj_h + 1);

    // Set up Pre Header.
    obj_p_h->gc_status = root ? GC_ROOT : GC_NEWLY_ADDED;

    // Set up normal Header.
    *(uint64_t *)&(obj_h->rt_len) = rt_len;
    *(uint64_t *)&(obj_h->da_size) = da_size;

    uint64_t i;
    for (i = 0; i < rt_len; i++) {
        rt[i] = NULL_VADDR;
    }

    if (hold) {
        return res;
    }

    ms_unlock(cs->ms, res.vaddr);
    res.paddr = NULL;

    return res;
}

malloc_res cs_malloc_object_p(collected_space *cs, uint64_t rt_len, 
        uint64_t da_size, uint8_t hold) {
    return cs_malloc_p(cs, rt_len, da_size, 0, hold);
}

static cs_root_id cs_store_root_unsafe(collected_space *cs, addr_book_vaddr vaddr) {
    // First we pop off the next free entry index from the root set.
    // Resizing if we need to.

    if (cs->free_head == UINT64_MAX) {
        // No free elements! ... resize!
        cs->free_head = cs->root_set_cap;
        cs->root_set_cap *= 2;

        cs->root_set = safe_realloc(cs->root_set,
                cs->root_set_cap * sizeof(root_set_entry));

        cs_root_id i;
        for (i = cs->free_head; i < cs->root_set_cap - 1; i++) {
            cs->root_set[i].allocated = 0;
            cs->root_set[i].next_free = i + 1;
        }

        // The final entry will point no where.
        cs->root_set[i].allocated = 0;
        cs->root_set[i].next_free = UINT64_MAX;
    }

    // Regardless of whether a resize occured... let's pop off!
    cs_root_id root_ind = cs->free_head;
    cs->free_head = cs->root_set[cs->free_head].next_free;

    // Finally, we store our root vaddr!
    cs->root_set[root_ind].allocated = 1;
    cs->root_set[root_ind].vaddr = vaddr;

    return root_ind;
}

cs_root_res cs_root(collected_space *cs, addr_book_vaddr vaddr) {
    cs_root_res res;

    safe_wrlock(&(cs->root_set_lock));
    obj_pre_header *obj_p_h = ms_get_write(cs->ms, vaddr);

    res.root_id = UINT64_MAX;

    if (obj_p_h->gc_status == GC_ROOT) {
        res.status_code = CS_ALREADY_ROOT;
    } else {
        obj_p_h->gc_status = GC_ROOT;
        res.root_id = cs_store_root_unsafe(cs, vaddr);

        res.status_code = CS_SUCCESS; 
    }

    ms_unlock(cs->ms, vaddr);
    safe_rwlock_unlock(&(cs->root_set_lock));

    return res;
}

cs_root_id cs_malloc_root(collected_space *cs, uint64_t rt_len, uint64_t da_size) {
    safe_wrlock(&(cs->root_set_lock));
    addr_book_vaddr root_vaddr = cs_malloc_p(cs, rt_len, da_size, 1, 0).vaddr;
    cs_root_id root_id = cs_store_root_unsafe(cs, root_vaddr);
    safe_rwlock_unlock(&(cs->root_set_lock));

    return root_id; 
}

cs_root_status_code cs_deroot(collected_space *cs, cs_root_id root_id) {
    addr_book_vaddr root_vaddr;

    cs_root_status_code res_code;

    safe_wrlock(&(cs->root_set_lock));

    if (root_id >= cs->root_set_cap) {
        res_code = CS_ROOT_ID_OUT_OF_BOUNDS;
    } else if (!(cs->root_set[root_id].allocated)) {
        res_code = CS_ROOT_ID_NOT_ALLOCATED;
    } else {
        res_code = CS_SUCCESS;

        root_vaddr = cs->root_set[root_id].vaddr;

        obj_pre_header *obj_p_h = ms_get_write(cs->ms, root_vaddr);
        obj_p_h->gc_status = GC_NEWLY_ADDED;
        ms_unlock(cs->ms, root_vaddr);

        // Finally, remove from root_set, push to front of 
        // free list.
        cs->root_set[root_id].allocated = 0;
        cs->root_set[root_id].next_free = cs->free_head;
        cs->free_head = root_id;
    }

    safe_rwlock_unlock(&(cs->root_set_lock));

    return res_code;
}

cs_get_root_res cs_get_root_vaddr(collected_space *cs, cs_root_id root_id) {
    cs_get_root_res res;
    res.root_vaddr = NULL_VADDR;

    safe_rdlock(&(cs->root_set_lock));

    if (root_id >= cs->root_set_cap) {
        res.status_code = CS_ROOT_ID_OUT_OF_BOUNDS;
    } else if (!(cs->root_set[root_id].allocated)) {
        res.status_code = CS_ROOT_ID_NOT_ALLOCATED;
    } else {
        res.status_code = CS_SUCCESS;
        res.root_vaddr = cs->root_set[root_id].vaddr;
    }

    safe_rwlock_unlock(&(cs->root_set_lock));

    return res;
}

obj_header *cs_get_read(collected_space *cs, addr_book_vaddr vaddr) {
    return (obj_header *)((obj_pre_header *)ms_get_read(cs->ms, vaddr) + 1);    
}

obj_header *cs_get_write(collected_space *cs, addr_book_vaddr vaddr) {
    return (obj_header *)((obj_pre_header *)ms_get_write(cs->ms, vaddr) + 1);    
}

void cs_unlock(collected_space *cs, addr_book_vaddr vaddr) {
    ms_unlock(cs->ms, vaddr);
}

static void obj_print(addr_book_vaddr v, void *paddr, void *ctx) {
    obj_pre_header *obj_p_h = paddr;
    obj_header *obj_h = (obj_header *)(obj_p_h + 1);

    safe_printf("--\n");

    safe_printf("Object @ Vaddr (%"PRIu64", %"PRIu64")\n",
            v.table_index, v.cell_index);

    safe_printf("Status: %s, RT Length: %"PRIu64", DA Size: %"PRIu64"\n",
            GC_STAT_STRINGS[obj_p_h->gc_status], obj_h->rt_len, obj_h->da_size);

    addr_book_vaddr *rt = (addr_book_vaddr *)(obj_h + 1);

    uint64_t i;
    for (i = 0; i < obj_h->rt_len; i++) {
        if (null_adb_addr(rt[i])) {
            safe_printf("rt[%"PRIu64"] = NULL\n", i);
        } else {
            safe_printf("rt[%"PRIu64"] = (%"PRIu64", %"PRIu64")\n", i,
                    rt[i].table_index, rt[i].cell_index);
        }
    }

    uint8_t *da = (uint8_t *)(rt + obj_h->rt_len);

    static const uint64_t ROW_LEN = 8;

    uint64_t j;
    for (i = 0; i < obj_h->da_size; ) {
        // Find Inclusive end index for this row.
        uint64_t end = i + ROW_LEN - 1;
        if (end >= obj_h->da_size) {
            end = obj_h->da_size - 1;
        }

        if (i == end) {
            safe_printf("da[%"PRIu64"] =", i);
        } else {
            safe_printf("da[%"PRIu64"...%"PRIu64"] =", i, end);
        }

        for (j = 0; j < ROW_LEN && i < obj_h->da_size; j++, i++) {
            safe_printf(" 0x%02X", da[i]);
        }

        safe_printf("\n");
    }
}

static inline void root_set_entry_print_unsafe(root_set_entry *entry) {
    if (!(entry->allocated)) {
        if (entry->next_free == UINT64_MAX) {
            safe_printf("(Free, Next: NULL)\n");
        } else {
            safe_printf("(Free, Next: %"PRIu64")\n", entry->next_free);
        }

        return;
    }

    // Allocated case.

    if (null_adb_addr(entry->vaddr)) {
        // Shouldn't really happen.
        safe_printf("(Allocated, Vaddr: NULL)\n");
    } else {
        safe_printf("(Allocated, Vaddr: (%"PRIu64", %"PRIu64"))\n",
                entry->vaddr.table_index, entry->vaddr.cell_index);
    }
}

void cs_print(collected_space *cs) {
    safe_rdlock(&(cs->root_set_lock));

    if (cs->free_head == UINT64_MAX) {
        safe_printf("Root Set (Cap: %"PRIu64", Free Head: NULL)\n",
                cs->root_set_cap);
    } else {
        safe_printf("Root Set (Cap: %"PRIu64", Free Head: %"PRIu64")\n",
                cs->root_set_cap, cs->free_head);
    }

    uint64_t i;
    root_set_entry *entry;

    for (i = 0; i < cs->root_set_cap; i++) {
        safe_printf("rs[%"PRIu64"] = ", i);
        root_set_entry_print_unsafe(cs->root_set + i);
    }

    ms_foreach(cs->ms, obj_print, NULL, 0);

    // NOTE: I decided that if we are going to print out
    // the full collected space, the root set should stay constant during 
    // this time. (... i.e. garbage collection should also not be occurring)
    //
    // Probs should never call cs_print in parallel with anything anyways.
    //
    safe_rwlock_unlock(&(cs->root_set_lock));
}

static void obj_unvisit(addr_book_vaddr v, void *paddr, void *ctx) {
    obj_pre_header *obj_p_h = paddr;

    if (obj_p_h->gc_status != GC_ROOT) {
        obj_p_h->gc_status = GC_UNVISITED;
    }
}

void cs_collect_garbage(collected_space *cs) {
    // NOTE: 
    // GC Definitions and Assumptions :
    // 
    // An object o1 is connected to another object o2 iff
    // the virtual address of o2 is in o1's reference table.
    //
    // Given a sequnce of objects o1, o2, o3, ... oN where N > 1, there is said
    // to be a path from o1 to oN iff oi is connected to oi+1 for all 1 <= i < N.
    //
    // Given an object o, o is either a root or it is not.
    //
    // Given an object s, s is reacheable iff there exists a root object o such that
    // there is a path from o to s.
    //
    // Assume that once an object is not reacheable, it is instantly GC'd
    //
    // GC algo notes:
    // During all phases of the algorithm normal objects can be added to the 
    // collected space. They will be marked as Newly Added.
    // Objects can also be arbitrary upgraded to root status and downgraded to
    // normal status. When an object is downgraded it is marked as Newly Added.
    //
    // 1 - Mark all non root objects as Unvisited (Painting White)
    //      * If an object is added after its address is iterated over, said
    //      object is simply ignored until the next GC cycle. Otherwise, it is 
    //      painted white and considered.
    //
    //      * If an object is randomly upgraded to root. Nothing bad happens.
    //
    //      * If an object is randomly downgraded, it is set as Newly Added.
    //      Then the 1st bullet points notes apply.
    //
    // 2 - Perform a graph search algorithm from the roots (Painting Black)
    //      * All roots at the start of this phase are added to a stack.
    //      If an arbitrary object is upgraded to a root and then disconnected 
    //      from other roots during this phase, there is an error possiblity.
    //      Objects connected to this new root could be GC'd as the new root
    //      was not added to the OG stack to begin with.
    //
    //      * Downgrading roots won't really matter. 
    //
    //      * For the first bullet point's reason, we will make the root set
    //      constant during this phase.
    //
    // 3 - Deleteing all objects which remain unvisited or "white" (Sweeping)
    //      * No problems here with object addition and root modification.
    //      If an object is white, it was not reacheable, just it is impossible
    //      said object is than turned into somethi
    //
   
    
    // Unvisit all non-root objects.
    ms_foreach(cs->ms, obj_unvisit, NULL, 1);
    
    // Implement graph search with a stack.
    //
    // Only 
    //
    util_bc *stack = new_broken_collection(get_chnl(cs), 
            sizeof(addr_book_vaddr), 100, 0);


    safe_rdlock(&(cs->root_set_lock));


    safe_rwlock_unlock(&(cs->root_set_lock));

    while (!bc_empty(stack)) {
        
    }
}

void cs_try_full_shift(collected_space *cs) {
    ms_try_full_shift(cs->ms);
}

