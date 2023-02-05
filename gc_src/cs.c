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
// Notes and Results on about ideas :
//
// If an object becomes unreachable, it is impossible for it every to become 
// reachable again. (That is, a user only has access to reachable objects and the
// values stored in their fields)
//
// An object should on be marked as "visited" when it is guaranteed all of the object's
// references (at the time of visitation) will be visited at some point in the
// future.
//
// When object is not visited, but guaranteed to be visited, it will be marked "in-progress".
//
// If GC is running, when the user requests the write lock on an object, we should
// guarantee the object is "visited" before the user gets write access to the object.
//
// When the user requests the write lock on an "unvisited" or "in-progress" object,
// the user thread will perform the visit.
//
// "paint black" can only end when the GC thread has no more objects left to visit
// and all user visits are complete.
// 
// The GC algorithm will use a stack to perform a DFS on the graph during the 
// "paint black" phase.
//
// A root is simply an object which is always marked "in-progress" at the begining of the
// "paint black" phase. If an object becomes a root during the "paint black" phase and 
// is not already "visited" or "in-progress", it will be marked "in-progress".
//
// The GC thread will be able to stop this phase when it is unoccupied and 
// the stack is empty.
//
// Reasoning :
// This stop condition would only be an issue iff when this condition is met
// a user were in the process of visiting an object.
// That is "paint black" finishes before all reachable objects are visited.
//
// CASE 1: the user is visiting an "in-progress" object.
// Since the object was marked "in-progress", it must've been added to the stack.
// Either it is still in the stack, or the GC thread just popped it off the stack and
// is now waiting, otherwise it would've been marked "visited".
//
// CASE 2: the user is visiting an "unvisited" object.
// This is key, no "visited" object is directly connected to an "unvisited" object.
// Thus, the existence of an "unvisited" object implies the existence of an "in-progress"
// object. Using reasoning from CASE 1, we know the existence of an "in-progress" object
// makes the stop condition impossible.
//
// Lastly, the operations described above ("GC thread waiting on the stack", "user visiting
// an object", etc..) will all require atomic GC operations. There will be a GC lock to ensure
// object visits happen sequentially.
//
// Objects Additions and Root Modifications at the begining of GC :
//
// All objects malloced after the end of "paint white" are guaranteed to
// be marked "newly added".
//
// When a user gets any form of lock on a "newly added" object, no visiting will 
// occur. If refernces are coming from other objects, we know said objects are 
// guaranteed to be visited or themselves be "newly added".
//
// The root set isn't all that special. Removing a root during a "paint black" phase
// will have no affect on the current GC cycle. GC will continue as if the deroot
// never occured.
//
// If an "unvisited" object is rooted, it will be placed in the GC stack and marked
// as "in-progress". This object will not need to be fully visited here as
// it is guaranteed its references aren't being modified.

typedef enum {
    GC_NEWLY_ADDED = 0,
    GC_UNVISITED,       // White.
    GC_IN_PROGRESS,     // Grey.    
    GC_VISITED,         // Black.
} gc_status_code;

typedef struct {
    gc_status_code gc_status;
} obj_pre_header;

static const uint64_t GC_STAT_STRINGS_LEN = 4;

static const char *GC_STAT_STRINGS[GC_STAT_STRINGS_LEN] = {
    "GC Newly Added",
    "GC Unvisited",
    "GC In Progress",
    "GC Visited",
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

    // This lock must be acquired whenevery any type of 
    // GC work is being executed.
    pthread_mutex_t gc_lock;

    // 1 When GC is going on.
    // 0 otherwise.
    uint8_t gc_in_progress;

    // 1 if the paint black phase is running.
    // 0 otherwise.
    uint8_t paint_black_in_progress;

    util_bc *gc_stack;

    // Fields for the root set.
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

    safe_mutex_init(&(cs->gc_lock), NULL);
    cs->gc_in_progress = 0;
    cs->gc_stack = new_broken_collection(chnl, sizeof(addr_book_vaddr), 40, 0);

    cs->root_set = safe_malloc(chnl, sizeof(root_set_entry) * 1);
    cs->root_set_cap = 1;
    uint64_t free_head = 0;
    cs->root_set[0].allocated = 0;
    cs->root_set[0].next_free = UINT64_MAX;

    return cs;
}

void delete_collected_space(collected_space *cs) {
    safe_mutex_destroy(&(cs->gc_lock));
    delete_broken_collection(cs->gc_stack);
    safe_free(cs->root_set);

    delete_mem_space(cs->ms);
    
    safe_free(cs);
}

// TODO... time to redo this all...

malloc_res cs_malloc_p(collected_space *cs, uint64_t rt_len,
        uint64_t da_size, uint8_t hold) {
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
    obj_p_h->gc_status = GC_NEWLY_ADDED;

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
    malloc_res res = cs_malloc_p(cs, rt_len, da_size, hold);

    // Skip past pre header for user facing function.
    if (res.paddr) {
        res.paddr =  (obj_pre_header *)res.paddr + 1;
    }

    return res;
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

cs_root_id cs_root(collected_space *cs, addr_book_vaddr vaddr) {
    safe_mutex_lock(&(cs->gc_lock));

    cs_root_id id = cs_store_root_unsafe(cs, vaddr);

    if (cs->paint_black_in_progress) {
        obj_pre_header *obj_p_h = ms_get_write(cs->ms, vaddr);        

        if (obj_p_h->gc_status == GC_UNVISITED) {
            obj_p_h->gc_status = GC_IN_PROGRESS; 
            bc_push_back(cs->gc_stack, &vaddr);
        }

        ms_unlock(cs->ms, vaddr);
    }

    safe_mutex_unlock(&(cs->gc_lock));

    return id;
}

cs_root_id cs_malloc_root(collected_space *cs, uint64_t rt_len, uint64_t da_size) {
    cs_root_id root_id;

    safe_mutex_lock(&(cs->gc_lock));
     
    // The is guaranteed to be "newly added".
    // Shouldn't matter if we are in GC or not.
    malloc_res res = cs_malloc_p(cs, rt_len, da_size, 0);
    root_id = cs_store_root_unsafe(cs, res.vaddr);
        
    safe_mutex_unlock(&(cs->gc_lock));

    return root_id; 
}

cs_root_status_code cs_deroot(collected_space *cs, cs_root_id root_id) {
    cs_root_status_code res_code;

    safe_mutex_lock(&(cs->gc_lock));
    addr_book_vaddr root_vaddr;

    if (root_id >= cs->root_set_cap) {
        res_code = CS_ROOT_ID_OUT_OF_BOUNDS;
    } else if (!(cs->root_set[root_id].allocated)) {
        res_code = CS_ROOT_ID_NOT_ALLOCATED;
    } else {
        res_code = CS_SUCCESS;

        root_vaddr = cs->root_set[root_id].vaddr;

        // NOTE: this has no affect on GC if the algorithm 
        // is mid "paint black" phase.
        // DO NOT need to edit the object itself.

        cs->root_set[root_id].allocated = 0;
        cs->root_set[root_id].next_free = cs->free_head;
        cs->free_head = root_id;
    }

    safe_mutex_unlock(&(cs->gc_lock));

    return res_code;
}

cs_get_root_res cs_get_root_vaddr(collected_space *cs, cs_root_id root_id) {
    cs_get_root_res res;
    res.root_vaddr = NULL_VADDR;

    // NOTE: in the future, consider giving the root set its own lock?

    safe_mutex_lock(&(cs->gc_lock));

    if (root_id >= cs->root_set_cap) {
        res.status_code = CS_ROOT_ID_OUT_OF_BOUNDS;
    } else if (!(cs->root_set[root_id].allocated)) {
        res.status_code = CS_ROOT_ID_NOT_ALLOCATED;
    } else {
        res.status_code = CS_SUCCESS;
        res.root_vaddr = cs->root_set[root_id].vaddr;
    }

    safe_mutex_unlock(&(cs->gc_lock));

    return res;
}

obj_header *cs_get_read(collected_space *cs, addr_book_vaddr vaddr) {
    return (obj_header *)((obj_pre_header *)ms_get_read(cs->ms, vaddr) + 1);    
}

obj_header *cs_get_write(collected_space *cs, addr_book_vaddr vaddr) {
    safe_mutex_lock(&(cs->gc_lock));

    if(!(cs->paint_black_in_progress)) {
        safe_mutex_unlock(&(cs->gc_lock));

        return (obj_header *)((obj_pre_header *)ms_get_write(cs->ms, vaddr) + 1);    
    }

    // From this point on we know paint black is going on.
    
    util_bc *temp_stack = NULL;
    uint64_t i;

    obj_pre_header *obj_p_h = ms_get_write(cs->ms, vaddr);
    obj_header *obj_h = (obj_header *)(obj_p_h + 1);
    addr_book_vaddr *rt = (addr_book_vaddr *)(obj_h + 1);

    // See if we must visit our object before aquiring
    // write lock.
    
    if (obj_p_h->gc_status == GC_UNVISITED || 
            obj_p_h->gc_status == GC_IN_PROGRESS) {
        temp_stack = new_broken_collection(get_chnl(cs), 
                sizeof(addr_book_vaddr), obj_h->rt_len + 1, 0); 

        for (i = 0; i < obj_h->rt_len; i++) {
            bc_push_back(temp_stack, rt + i); 
        }

        // Only can set this here because we have the GC lock.
        // This object won't actually be "visited" until the following 
        // while loop executes.
        obj_p_h->gc_status = GC_VISITED;
    }

    ms_unlock(cs->ms, vaddr);

    // Check if a visit is needed.
    if (temp_stack) {
        addr_book_vaddr child_vaddr;
        obj_pre_header *c_obj_p_h;

        while (!bc_empty(temp_stack)) {
            bc_pop_back(temp_stack, &child_vaddr);
            
            c_obj_p_h = ms_get_write(cs->ms, child_vaddr);

            if (c_obj_p_h->gc_status == GC_UNVISITED) {
                bc_push_back(cs->gc_stack, &child_vaddr);

                c_obj_p_h->gc_status = GC_IN_PROGRESS;
            }

            ms_unlock(cs->ms, child_vaddr);
        }
        
        delete_broken_collection(temp_stack);
    }

    safe_mutex_unlock(&(cs->gc_lock));

    // DONE
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
    // Halt GC while printing.
    safe_mutex_lock(&(cs->gc_lock));

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
    safe_mutex_unlock(&(cs->gc_lock));
}

static void obj_unvisit(addr_book_vaddr v, void *paddr, void *ctx) {
    obj_pre_header *obj_p_h = paddr;

    // Won't need to aquire GC lock as this is the paint white phase.
    
    obj_p_h->gc_status = GC_UNVISITED;
}

void cs_collect_garbage(collected_space *cs) {
    safe_mutex_lock(&(cs->gc_lock));  

    if (cs->gc_in_progress) {
        safe_mutex_unlock(&(cs->gc_lock));
        return;
    }

    // Only one GC can happen at a time.
    cs->gc_in_progress = 1;
    cs->paint_black_in_progress = 0;
    
    safe_mutex_unlock(&(cs->gc_lock));
    
    // Unvisit all non-root objects.
    ms_foreach(cs->ms, obj_unvisit, NULL, 1);

    safe_mutex_lock(&(cs->gc_lock));  

    cs->paint_black_in_progress = 1;

    uint64_t root_i;
    addr_book_vaddr root_vaddr;
    obj_pre_header *root_obj_p_h;

    for (root_i = 0; root_i < cs->root_set_cap; root_i++) {
        if (cs->root_set[root_i].allocated) {
            root_vaddr = cs->root_set[root_i].vaddr;

            // Place all roots in the gc stack.
            bc_push_back(cs->gc_stack, &root_vaddr);

            root_obj_p_h = ms_get_write(cs->ms, root_vaddr);
            root_obj_p_h->gc_status = GC_IN_PROGRESS;
            ms_unlock(cs->ms, root_vaddr);
        }
    }

    // TODO... think more about what this will look like here...
    // Maybe switch the order of gc_lock and object locks???
    // Could be better. My brain is fried regardless.
    
    safe_mutex_unlock(&(cs->gc_lock));

}

void cs_try_full_shift(collected_space *cs) {
    ms_try_full_shift(cs->ms);
}

