#include "gc.h"
#include "ms.h"
#include "virt.h"
#include "../core_src/mem.h"
#include "../core_src/thread.h"

typedef enum {
    GC_UNVISITED,
    GC_VISITED,
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

static addr_book_vaddr ms_new_obj(mem_space *ms, uint64_t rt_len, 
        uint64_t da_size) {
    // TODO... rewrite this below...
    
    uint64_t obj_size = sizeof(obj_header) + 
        (rt_len * sizeof(addr_book_vaddr)) +
        (da_size * sizeof(uint8_t));

    addr_book_vaddr v = ms_malloc(ms, obj_size);

    obj_header *obj_h = ms_get_write(ms, v);

    obj_h->gc_status = GC_NEWLY_ADDED;
    obj_h->rt_len = rt_len;
    obj_h->da_size = da_size;

    addr_book_vaddr *rt = (addr_book_vaddr *)(obj_h + 1);

    // NULL out reference table.
    uint64_t i;
    for (i = 0; i < rt_len; i++) {
        rt[i] = NULL_VADDR;
    }

    // Don't worry about data array...

    ms_unlock(ms, v);

    return v;
}

gc_space *new_gc_space_seed(uint64_t chnl, uint64_t seed, 
        uint64_t adb_t_cap, uint64_t mb_m_bytes,
        uint64_t root_rt_len) {

    mem_space *ms = new_mem_space_seed(chnl, seed, adb_t_cap, mb_m_bytes); 
    addr_book_vaddr root = ms_new_obj(ms, root_rt_len, 0);

    gc_space *gc = safe_malloc(1, sizeof(gc_space));

    *(mem_space **)&(gc->ms) = ms;
    *(addr_book_vaddr *)&(gc->root) = root;

    return gc;
}

void delete_gc_space(gc_space *gc) {
    delete_mem_space(gc->ms); 
    safe_free(gc);
}



