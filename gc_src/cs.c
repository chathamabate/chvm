#include "cs.h"
#include "ms.h"
#include "virt.h"
#include "../core_src/mem.h"
#include "../core_src/thread.h"

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

static uint64_t cs_pop_root_entry_unsafe(collected_space *cs) {
    // Assume we have the necessary lock on the root set.
    
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

    // NOTE: after this returns, the given root entry will not
    // be marked as allocated. It should not be marked as allocated
    // until it holds a valid virtual address.
    //
    // This function simply removes it from the free list.

    return root_ind;
}

static inline malloc_res cs_malloc_object_and_hold(collected_space *cs, 
        gc_status_code gc_status, uint64_t rt_len, uint64_t da_size) {
    malloc_res res =  ms_malloc_and_hold(cs->ms, sizeof(obj_header) + 
            (rt_len * sizeof(addr_book_vaddr)) +
            (da_size * sizeof(uint8_t)));

    obj_header *obj_h = res.paddr;

    obj_h->rt_len = rt_len;
    obj_h->da_size = da_size;

    obj_h->gc_status = gc_status;

    // TODO add nulled out reference table!
}

uint64_t cs_new_root(collected_space *cs, uint64_t rt_len) {
    uint64_t root_ind;

    safe_wrlock(&(cs->root_set_lock));
    root_ind = cs_pop_root_entry_unsafe(cs);

    // TODO finish this all up...

    safe_rwlock_unlock(&(cs->root_set_lock));

    return 0;
}

uint64_t cs_copy_root(collected_space *cs, uint64_t root_ind, uint64_t rt_len) {
    return 0;
}

void cs_remove_root(collected_space *cs, uint64_t root_ind) {
    // TODO
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

