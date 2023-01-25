#include "cs.h"
#include "mb.h"
#include "ms.h"
#include "virt.h"
#include "../core_src/mem.h"
#include "../core_src/thread.h"
#include "../core_src/sys.h"

typedef enum {
    GC_NEWLY_ADDED,

    GC_UNVISITED,
    GC_VISITED,

    GC_ROOT,
} gc_status_code;

typedef struct {} obj;

typedef struct {
    gc_status_code gc_status;     
    
    // The number of references in the reference table.
    uint64_t rt_len;

    // The number of bytes in the data array.
    uint64_t da_size;
} obj_header;

static inline addr_book_vaddr *obj_get_rt(obj *o) {
    return (addr_book_vaddr *)((obj_header *)o + 1);
}

static inline uint8_t *obj_get_da(obj *o) {
    obj_header *h = (obj_header *)o;
    return (uint8_t *)((addr_book_vaddr *)(h + 1) + h->rt_len);
}

typedef struct {
    uint8_t allocated;

    union {
        uint64_t next_free;
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
    // However, it will only have a reference table, no data.
    // The user will always use root objects to access other
    // arbitrary objects.
    pthread_rwlock_t root_set_lock;

    uint64_t root_set_cap;

    // If this is UINT64T_MAX, the free list is empty.
    uint64_t free_head;
    root_set_entry *root_set;
};

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

    return cs;
}

void delete_collected_space(collected_space *cs) {
    safe_free(cs->root_set);
    delete_mem_space(cs->ms);

    safe_free(cs);
}

static inline malloc_res cs_malloc_object_p(collected_space *cs, 
        gc_status_code gc_status, uint64_t rt_len, uint64_t da_size, uint8_t hold) {

    malloc_res res =  ms_malloc_and_hold(cs->ms, sizeof(obj_header) + 
            (rt_len * sizeof(addr_book_vaddr)) +
            (da_size * sizeof(uint8_t)));

    obj_header *obj_h = res.paddr;

    obj_h->rt_len = rt_len;
    obj_h->da_size = da_size;

    obj_h->gc_status = gc_status;

    addr_book_vaddr *rt = (addr_book_vaddr *)(obj_h + 1);
    
    uint64_t i;
    for (i = 0; i < rt_len; i++) {
        rt[i] = NULL_VADDR;
    }

    if (hold) {
        return res;
    }

    // Unlock virtual address.
    ms_unlock(cs->ms, res.vaddr);
    res.paddr = NULL; // Throw out physical address.

    return res;
}


static inline addr_book_vaddr cs_malloc_object(collected_space *cs, 
        gc_status_code gc_status, uint64_t rt_len, uint64_t da_size) {
    return cs_malloc_object_p(cs, gc_status, rt_len, da_size, 0).vaddr;
}

static inline malloc_res cs_malloc_object_and_hold(collected_space *cs, 
        gc_status_code gc_status, uint64_t rt_len, uint64_t da_size) {
    return cs_malloc_object_p(cs, gc_status, rt_len, da_size, 1);
}

static uint64_t cs_store_root(collected_space *cs, addr_book_vaddr root_vaddr) {
    safe_wrlock(&(cs->root_set_lock));
    
    // First we pop off the next free entry index from the root set. 
    // Resizing if we need to.
    
    if (cs->free_head == UINT64_MAX) {
        // No free elements! ... resize!
        cs->free_head = cs->root_set_cap;
        cs->root_set_cap *= 2;

        cs->root_set = safe_realloc(cs->root_set, 
                cs->root_set_cap * sizeof(root_set_entry));
        
        uint64_t i;
        for (i = cs->free_head; i < cs->root_set_cap - 1; i++) {
            cs->root_set[i].allocated = 0;
            cs->root_set[i].next_free = i + 1;
        }

        // The final entry will point no where.
        cs->root_set[i].allocated = 0;
        cs->root_set[i].next_free = UINT64_MAX;
    }

    // Regardless of whether a resize occured... let's pop off!
    uint64_t root_ind = cs->free_head;
    cs->free_head = cs->root_set[cs->free_head].next_free;

    // Finally, we store our root vaddr!
    cs->root_set[root_ind].allocated = 1;
    cs->root_set[root_ind].vaddr = root_vaddr;

    safe_rwlock_unlock(&(cs->root_set_lock));

    return root_ind;
}

uint64_t cs_new_root(collected_space *cs, uint64_t rt_len) {
    // NOTE: One day we made add error checking universally.
    // I just don't know if it is needed at this moment.
    //if (rt_len == 0) {
    //    error_logf(1, 1, "cs_new_root: reference table length must be non-zero");
    //}

    // Create our root object, store it in the root set.
    addr_book_vaddr root_vaddr = cs_malloc_object(cs, GC_ROOT, rt_len, 0);
    uint64_t root_ind = cs_store_root(cs, root_vaddr);

    return root_ind;
}

uint64_t cs_copy_root(collected_space *cs, uint64_t src_root_ind, uint64_t dest_rt_len) {
    addr_book_vaddr src_root_vaddr;

    safe_rdlock(&(cs->root_set_lock)); 
    src_root_vaddr = cs->root_set[src_root_ind].vaddr;
    safe_rwlock_unlock(&(cs->root_set_lock));

    // NOTE: Given the underlying memory space is only being used through these 
    // calls... It is impossible another thread would have access to 
    // the new root's vaddr. Thus, a dead lock should be impossible here...

    obj_header *src_root_h = ms_get_read(cs->ms, src_root_vaddr);
    uint64_t src_rt_len = src_root_h->rt_len;
    addr_book_vaddr *src_rt = (addr_book_vaddr *)(src_root_h + 1);

    malloc_res dest_root_res = cs_malloc_object_and_hold(cs, GC_ROOT, dest_rt_len, 0);
    obj_header *dest_root_h = dest_root_res.paddr;
    addr_book_vaddr *dest_rt = (addr_book_vaddr *)(dest_root_h + 1);

    // Now we have access to both reference tables.
    // Src's in read mode, and dest's in write mode... let's do the copy.
    uint64_t i;
    for (i = 0; i < src_rt_len && i < dest_rt_len; i++) {
        dest_rt[i] = src_rt[i];
    }

    ms_unlock(cs->ms, dest_root_res.vaddr);

    ms_unlock(cs->ms, src_root_vaddr);

    return cs_store_root(cs, dest_root_res.vaddr);
}

void cs_remove_root(collected_space *cs, uint64_t root_ind) {
    // Once again, we are assuming that a user is not removing a root
    // until it is certain that it is no longer in use by any other threads.
    // This call should never be in parallel with another one of these 
    // calls to the same root.
    
    addr_book_vaddr root_vaddr;

    // First, let's take our root out of the root set and add it to 
    // the free list.
    safe_wrlock(&(cs->root_set_lock));
    root_set_entry *root_entry = cs->root_set + root_ind;

    // Save the root vaddr.
    root_vaddr = root_entry->vaddr;

    root_entry->allocated = 0;
    root_entry->next_free = cs->free_head;

    cs->free_head = root_ind;
    safe_rwlock_unlock(&(cs->root_set_lock));

    // Finally, let's free our root from the mem_space.
    ms_free(cs->ms, root_vaddr);
}

void cs_new_obj(collected_space *cs, uint64_t root_ind, uint64_t offset,
        uint64_t rt_len, uint64_t da_size) {
    // TODO.
}

void cs_move_reference(collected_space *cs, uint64_t root_ind,
        uint64_t dest_offset, uint64_t src_offset) {
    // TODO
}

void cs_load_reference(collected_space *cs, uint64_t root_ind,
        uint64_t dest_offset, uint64_t src_offset, uint64_t src_rt_offset) {
    // TODO
}

void cs_store_reference(collected_space *cs, uint64_t root_ind,
        uint64_t dest_offset, uint64_t dest_rt_offset, uint64_t src_offset) {
    // TODO
}

void cs_read_data(collected_space *cs, uint64_t root_ind, 
        uint64_t src_offset, uint64_t src_da_offset, uint64_t len, void *output_buf) {
    // TODO
}

void cs_write_data(collected_space *cs, uint64_t root_ind,
        uint64_t dest_offset, uint64_t dest_da_offset, uint64_t len, void *input_buf) {
    // TODO
}

void cs_collect_garbage(collected_space *cs) {
    // TODO
}

void cs_try_full_shift(collected_space *cs) {
    // TODO
}

