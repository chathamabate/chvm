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
#include <sys/_pthread/_pthread_rwlock_t.h>
#include <time.h>

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
// NEW GC tri-color Algo Design:
//
// The garbage collector will have two stacks.
// A visit-stack and an in-progress-stack.
//
// The visit-stack will be guaranteed to only hold objects which are marked "in-progress" or 
// "visited". Objects popped of this stack which are marked "visited" will be ignored.
// Objects popped off this stack which are marked "in-progress" will have all their references
// pushed onto the in-progress-stack... and will be marked "visited".
//
// The in-progress-stack can hold any type of object. Objects popped off this stack which
// are marked "unvisited" will be added to the visit-stack and marked "in-progress".
// 
// At the beginning of the "paint black" phase of the algorithm, all roots will be pushed onto
// the in-progress-stack. It is important to realize that if an object is rooted in
// the middle of the "paint black" phase, nothing really changes. There is no need to
// add said object to the in-progress stack as said object is guaranteed to have been
// reachable at the beginning of "paint black" and thus will be visited.
//
// When the user requests the write lock on an object marked "in-progress" or "unvisited",
// the object will be visited then and there by the user thread. That is all of the object's
// references will be place on the in-progress stack, and the object will be marked "visited".
//
// The "paint black" phase ends when the GC thread is free and both the in-progress-stack 
// and visit-stack are empty.
//
// Does this exit condition guarantee all reachable objects have been marked "viisted"?
// 
// * At any time a user could be visiting an object.
//
// * Only the GC thread marks objects "in-progress". When an object is marked "in-progress" it is 
// guaranteed to have been added to the visit-stack. Which then guarantees the GC thread will 
// inspect that object and "visit" it if the user thread is yet to do so.
// 
// * If an "in-progress" object exists, one of these cases must be true :
//      1) in the visit-stack and not being visited                 (visit-stack non-empty)
//      2) in the visit-stack and being visited by a user thread    (visit-stack non-empty)
//      3) being visited by the user and waited on by the GC thread (GC thread busy)
//      4) being visited by the GC thread                           (GC thread busy)
//
// * => If an "in-progress" object exists, the algorithm will not stop.
//
// * First, from above, we know if an "in-progress" object exists, the algorithm will
// still be running. So, assume one does not exist. If a reachable "unvisited" object exists,
// there must exist a path from the root to the object. As all roots are eventually visited,
// and our object is reachable, there must exist a "visited" object v which references an 
// "unvisited" object u. Since v is "visited", u must have been added to the in-progress stack.
// This means one of the following must be true :
//      1) u is in the in-progress-stack                                        (in-progress-stack non-empty)
//      2) u is in the in-progress-stack and being visited by a user thread     (in-progress-stack non-empty)
//      3) u is being visited by a user thread and waited on by the GC thread   (GC thread busy)
//      4) u is being moved to the visit-stack by the GC thread                 (GC thread busy)
//
// * => If a reachable "unvisited" object exists, the algorithm will not stop.
//
// * Lastly, these points only prove the algorithm will be running when there are still objects left
// to visit. It does not prove the correctness of the algorithm or that it will ever stop running.
// I believe these results are intuitive though as all objects can only be "visited" once 
// (allowing for termination). Additionally, the search algorithm is DFS, which is well known.
//
// Notes on User Safety :
//
// The above points took extra detail to account for the fact that the user thread can "visit" objects.
// Why do I even give this responsibility to the user?
//
// This is to maintain the condition that users only ever have write access to objects which 
// have been visited. If this were not the case, the user could transfer a reference from 
// an "unvisited" object to a "visited" object, then remove the reference from the "unvisited" 
// object. If this were to happen, it would be possible for a reachable object to never
// be visited!

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

typedef enum {
    GC_WORKER_ON = 0,
    GC_WORKER_STARTING,
    GC_WORKER_STOPPING,
    GC_WORKER_OFF,
} gc_worker_status_code;

struct collected_space_struct {
    mem_space * const ms;

    // Lock for accessing the progress fields.
    pthread_rwlock_t gc_stat_lock;
    struct {
        gc_worker_status_code gc_worker_stat : 2;

        // Consider reworking below two fields.
        uint8_t gc_in_progress : 1;
        uint8_t paint_black_in_progress : 1;
    };
    
    // Thread ID used when gc worker is on. 
    // Otherwise it has an undefined value.
    pthread_t gc_thread;

    pthread_mutex_t in_progress_stack_lock;
    util_bc *in_progress_stack;

    // No need for a lock on this guy.
    util_bc *visit_stack;

    pthread_rwlock_t root_set_lock;

    // Fields for the root set.
    uint64_t root_set_cap;

    // If this is UINT64T_MAX, the free list is empty.
    cs_root_id free_head;
    root_set_entry *root_set;
};


collected_space *new_collected_space_seed(uint64_t chnl, uint64_t seed, 
        uint64_t adb_t_cap, uint64_t mb_m_bytes) {
    collected_space *cs = safe_malloc(chnl, sizeof(collected_space));

    // Create our underlying memory space.
    *(mem_space **)&(cs->ms) = 
        new_mem_space_seed(chnl, seed, adb_t_cap, mb_m_bytes);

    safe_rwlock_init(&(cs->gc_stat_lock), NULL);
    cs->gc_worker_stat = GC_WORKER_OFF;
    cs->gc_in_progress = 0;
    cs->paint_black_in_progress = 0;

    safe_mutex_init(&(cs->in_progress_stack_lock), NULL);
    cs->in_progress_stack = new_broken_collection(chnl, sizeof(addr_book_vaddr), 100, 0);
    cs->visit_stack = new_broken_collection(chnl, sizeof(addr_book_vaddr), 100, 0);

    safe_rwlock_init(&(cs->root_set_lock), NULL);
    cs->root_set = safe_malloc(chnl, sizeof(root_set_entry) * 1);
    cs->root_set_cap = 1;
    cs->free_head = 0;
    cs->root_set[0].allocated = 0;
    cs->root_set[0].next_free = UINT64_MAX;

    return cs;
}

void delete_collected_space(collected_space *cs) {
    safe_rwlock_destroy(&(cs->gc_stat_lock));
    safe_mutex_destroy(&(cs->in_progress_stack_lock));
    safe_rwlock_destroy(&(cs->root_set_lock));

    delete_broken_collection(cs->in_progress_stack);
    delete_broken_collection(cs->visit_stack);

    safe_free(cs->root_set);
    delete_mem_space(cs->ms);
    
    safe_free(cs);
}

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

malloc_obj_res cs_malloc_object_p(collected_space *cs, uint64_t rt_len, 
        uint64_t da_size, uint8_t hold) {
    malloc_res mr = cs_malloc_p(cs, rt_len, da_size, hold);

    malloc_obj_res mor = {
        .vaddr = mr.vaddr,
        .i = {
            .h = NULL,
            .rt = NULL,
            .da = NULL,
        },
    };

    if (mr.paddr) {
        // If we held, convert object header into object index!
        obj_header *obj_h = (obj_header *)((obj_pre_header *)mr.paddr + 1);
        mor.i = obj_h_to_index(obj_h);
    } 

    return mor;
}

uint8_t cs_allocated(collected_space *cs, addr_book_vaddr vaddr) {
    return ms_allocated(cs->ms, vaddr);
}

static inline uint8_t cs_gc_in_progress(collected_space *cs) {
    uint8_t gc_in_progress;

    safe_rdlock(&(cs->gc_stat_lock));
    gc_in_progress = cs->gc_in_progress;
    safe_rwlock_unlock(&(cs->gc_stat_lock));

    return gc_in_progress;
}

static inline uint8_t cs_paint_black_in_progress(collected_space *cs) {
    uint8_t paint_black_in_progress;

    safe_rdlock(&(cs->gc_stat_lock));
    paint_black_in_progress = cs->paint_black_in_progress;
    safe_rwlock_unlock(&(cs->gc_stat_lock));

    return paint_black_in_progress;
}

static inline void cs_set_paint_black_in_progress(collected_space *cs, uint8_t ip) {
    safe_wrlock(&(cs->gc_stat_lock));
    cs->paint_black_in_progress = ip;
    safe_rwlock_unlock(&(cs->gc_stat_lock));
}

cs_root_id cs_root(collected_space *cs, addr_book_vaddr vaddr) {
    safe_wrlock(&(cs->root_set_lock));

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

    safe_rwlock_unlock(&(cs->root_set_lock));

    // All we do is add the root to the root set, nothing else.

    return root_ind;
}

cs_root_id cs_malloc_root(collected_space *cs, uint64_t rt_len, uint64_t da_size) {
    cs_root_id root_id;

    // The concept here is that while an object's lock is held, it cannot
    // be GC'd. So, we don't release the object's lock until it has been
    // added to the root set!
    addr_book_vaddr vaddr = cs_malloc_p(cs, rt_len, da_size, 1).vaddr;
    root_id = cs_root(cs, vaddr);
    cs_unlock(cs, vaddr);

    return root_id;
}

cs_root_status_code cs_deroot(collected_space *cs, cs_root_id root_id) {
    cs_root_status_code res_code;

    safe_wrlock(&(cs->root_set_lock));
    addr_book_vaddr root_vaddr;

    if (root_id >= cs->root_set_cap) {
        res_code = CS_ROOT_ID_OUT_OF_BOUNDS;
    } else if (!(cs->root_set[root_id].allocated)) {
        res_code = CS_ROOT_ID_NOT_ALLOCATED;
    } else {
        res_code = CS_SUCCESS;

        // NOTE: this has no affect on GC if the algorithm 
        // is mid "paint black" phase.
        // DO NOT need to edit the object itself.

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

static inline void cs_visit_obj(collected_space *cs, obj_pre_header *obj_p_h) {
    obj_header *obj_h = (obj_header *)(obj_p_h + 1);
    addr_book_vaddr *rt = (addr_book_vaddr *)(obj_h + 1);

    safe_mutex_lock(&(cs->in_progress_stack_lock));

    uint64_t ref_i;
    for (ref_i = 0; ref_i < obj_h->rt_len; ref_i++) {
        if (!null_adb_addr(rt[ref_i])) {
            bc_push_back(cs->in_progress_stack, rt + ref_i); 
        }
    }

    safe_mutex_unlock(&(cs->in_progress_stack_lock));

    obj_p_h->gc_status = GC_VISITED;

}

obj_header *cs_get_write(collected_space *cs, addr_book_vaddr vaddr) {
    // So, if paint black is going on... gotta make some changes I guess??
    // When do these stages actually begin???

    obj_pre_header *obj_p_h = (obj_pre_header *)ms_get_write(cs->ms, vaddr);
    obj_header *obj_h = (obj_header *)(obj_p_h + 1);

    if (obj_p_h->gc_status == GC_VISITED || obj_p_h == GC_NEWLY_ADDED) {
        return obj_h;
    }

    if (cs_paint_black_in_progress(cs)) {
        // NOTE: if we make it here, this means we have the lock on an 
        // unvisited/in-progress object, and paint black is occuring.
        // We know that because we have the object lock, it is impossible
        // paint black has ended since calling cs_paint_black_in_progress.
        
        cs_visit_obj(cs, obj_p_h);
    } 

    return (obj_header *)(obj_p_h + 1);
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

uint64_t cs_count(collected_space *cs) {
    return ms_count(cs->ms);
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

    safe_rwlock_unlock(&(cs->root_set_lock));

    // NOTE: probs shouldn't call this function in parallel with anything else,
    // Nothing will break, but the results might be funky.

    ms_foreach(cs->ms, obj_print, NULL, 0);
}

static void obj_unvisit(addr_book_vaddr v, void *paddr, void *ctx) {
    obj_pre_header *obj_p_h = paddr;

    obj_p_h->gc_status = GC_UNVISITED;
}

static uint8_t obj_reachable(addr_book_vaddr v, void *paddr, void *ctx) {
    obj_pre_header *obj_p_h = paddr;
    return obj_p_h->gc_status != GC_UNVISITED;
}

uint64_t cs_collect_garbage(collected_space *cs) {
    safe_wrlock(&(cs->gc_stat_lock));

    if (cs->gc_in_progress) {
        safe_rwlock_unlock(&(cs->gc_stat_lock));

        return 0;
    }

    cs->gc_in_progress = 1;

    safe_rwlock_unlock(&(cs->gc_stat_lock));

    // Paint White.
    ms_foreach(cs->ms, obj_unvisit, NULL, 1);  

    safe_rdlock(&(cs->root_set_lock)); 
    // Once we have acquired the root set lock.
    // The "paint black" phase has officially started.
    // The roots at this point in time will be the only roots considered.
    //
    // NOTE: If a root is added after this point in the paint black phase,
    // it is implied that it was reachable when paint black started.
    // Thus it will not be GC'd.
    cs_set_paint_black_in_progress(cs, 1);

    safe_mutex_lock(&(cs->in_progress_stack_lock));
    uint64_t root_i;
    root_set_entry entry;
    for (root_i = 0; root_i < cs->root_set_cap; root_i++) {
        entry = cs->root_set[root_i];

        if (entry.allocated) {
            bc_push_back(cs->in_progress_stack, &(entry.vaddr));
        }
    }
    safe_mutex_unlock(&(cs->in_progress_stack_lock));
    
    safe_rwlock_unlock(&(cs->root_set_lock));

    // Now, time for DFS...
    
    // We stop when an iteration which did no work occurs.
    uint8_t productive_iteration = 1;

    addr_book_vaddr vaddr;

    obj_pre_header *obj_p_h;
    obj_header *obj_h;
    addr_book_vaddr *rt;

    while (productive_iteration) {
        productive_iteration = 0;

        while (1) {
            safe_mutex_lock(&(cs->in_progress_stack_lock));
            if (bc_empty(cs->in_progress_stack)) {
                safe_mutex_unlock(&(cs->in_progress_stack_lock));

                break;
            }

            bc_pop_back(cs->in_progress_stack, &vaddr);
            safe_mutex_unlock(&(cs->in_progress_stack_lock));

            obj_p_h = ms_get_write(cs->ms, vaddr);

            // Transfer to visit stack if needed.
            if (obj_p_h->gc_status == GC_UNVISITED) {
                obj_p_h->gc_status = GC_IN_PROGRESS; 
                bc_push_back(cs->visit_stack, &vaddr);
            }

            ms_unlock(cs->ms, vaddr);

            productive_iteration = 1;
        }

        while (!bc_empty(cs->visit_stack)) {
            bc_pop_back(cs->visit_stack, &vaddr);

            obj_p_h = ms_get_write(cs->ms, vaddr);

            if (obj_p_h->gc_status == GC_IN_PROGRESS) {
                // ... as oppposed to already being visited
                // by the user.
                
                cs_visit_obj(cs, obj_p_h);
            }

            ms_unlock(cs->ms, vaddr);

            productive_iteration = 1;
        }
    }

    cs_set_paint_black_in_progress(cs, 0);

    // Finally time for "sweep" phase.
    uint64_t filtered = ms_filter(cs->ms, obj_reachable, NULL);

    safe_wrlock(&(cs->gc_stat_lock));
    cs->gc_in_progress = 0;
    safe_rwlock_unlock(&(cs->gc_stat_lock));

    return filtered;
}

typedef struct {
    collected_space *cs;

    const gc_worker_spec *spec;
} cs_gc_worker_arg;

static void *cs_gc_worker(void *arg) {
    cs_gc_worker_arg *gc_arg = arg;

    collected_space *cs = gc_arg->cs;
    const gc_worker_spec *spec = gc_arg->spec;

    // We can delete our gc_arg once we have extracted
    // the needed info.
    safe_free(gc_arg);

    // If the thread has started, it is guaranteed the status
    // is GC_STARTING, GC_ON, or GC_STOPPING.
    // This will be the only place where the status is cchanged to GC_OFF.

    uint64_t free_count = 0;
    uint8_t stopping = 0;
    
    while (!stopping) {
        // Here, GC is running!

        free_count += cs_collect_garbage(cs);

        if (spec->shift && free_count >= spec->shift_trigger) {
            free_count = 0;
            cs_try_full_shift(cs);
        }

        if (spec->delay) {
            // NOTE : No signal interrupt protection here...
            nanosleep(spec->delay, NULL);
        }

        safe_rdlock(&(cs->gc_stat_lock));
        stopping = cs->gc_worker_stat == GC_WORKER_STOPPING;
        safe_rwlock_unlock(&(cs->gc_stat_lock));
    }

    safe_wrlock(&(cs->gc_stat_lock));
    cs->gc_worker_stat = GC_WORKER_OFF;
    safe_rwlock_unlock(&(cs->gc_stat_lock));

    return NULL;
}

uint8_t cs_start_gc(collected_space *cs, const gc_worker_spec *spec) {
    safe_wrlock(&(cs->gc_stat_lock));

    if (cs->gc_worker_stat != GC_WORKER_OFF) {
        safe_rwlock_unlock(&(cs->gc_stat_lock));

        return 1;
    }

    cs->gc_worker_stat = GC_WORKER_STARTING;
    
    safe_rwlock_unlock(&(cs->gc_stat_lock));

    cs_gc_worker_arg *gc_arg = safe_malloc(get_chnl(cs), sizeof(cs_gc_worker_arg));
    gc_arg->cs = cs;
    gc_arg->spec = spec;

    safe_pthread_create(&(cs->gc_thread), NULL, cs_gc_worker, gc_arg);

    safe_wrlock(&(cs->gc_stat_lock));
    cs->gc_worker_stat = GC_WORKER_ON;
    safe_rwlock_unlock(&(cs->gc_stat_lock));

    return 0;
}

uint8_t cs_stop_gc(collected_space *cs) {
    gc_worker_status_code stat;

    safe_wrlock(&(cs->gc_stat_lock));
    if (cs->gc_worker_stat != GC_WORKER_ON) {
        safe_rwlock_unlock(&(cs->gc_stat_lock)); 

        return 1;
    }

    cs->gc_worker_stat = GC_WORKER_STOPPING;
    safe_rwlock_unlock(&(cs->gc_stat_lock)); 

    // Always join to reap zombie thread.
    safe_pthread_join(cs->gc_thread, NULL);

    return 0;
}

void cs_try_full_shift(collected_space *cs) {
    ms_try_full_shift(cs->ms);
}

