#include "cs.h"
#include "ms.h"
#include "virt.h"
#include "../core_src/mem.h"
#include "../core_src/thread.h"

typedef enum {

    GC_UNVISITED,
    GC_VISITED,

    GC_ROOT,

    GC_NEWLY_ADDED,

} gc_status_code;

typedef struct {} obj;

typedef struct {
    gc_status_code gc_status;     
    
    // The number of references in the reference table.
    uint64_t rt_len;

    // The number of bytes in the data array.
    uint64_t da_size;
} obj_header;

typedef union {
    uint64_t next_free;
    addr_book_vaddr vaddr;
} root_set_entry;

struct gc_space_struct {
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

    // If this is UINT64T_MAX, the free list is empty.
    uint64_t free_head;
    root_set_entry *root_set;
};

collected_space *new_collected_space_seed(uint64_t chnl, uint64_t seed, 
        uint64_t adb_t_cap, uint64_t mb_m_bytes) {
    return NULL;
}

void delete_collected_space(collected_space *cs) {
    // TODO
}

uint64_t cs_new_root(collected_space *cs, uint64_t rt_len) {
    return 0;
}

uint64_t cs_copy_root(collected_space *cs, uint64_t root_ind, uint64_t rt_len) {
    return 0;
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

