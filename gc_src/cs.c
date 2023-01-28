#include "cs.h"
#include "mb.h"
#include "ms.h"
#include "virt.h"
#include "../core_src/mem.h"
#include "../core_src/thread.h"
#include "../core_src/sys.h"
#include "../core_src/io.h"

#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>

typedef enum {
    GC_NEWLY_ADDED = 0,

    GC_UNVISITED,
    GC_VISITED,

    GC_ROOT,
} gc_status_code;

static const uint64_t GC_STAT_STRINGS_LEN = 4;

static const char *GC_STAT_STRINGS[GC_STAT_STRINGS_LEN] = {
    "GC Newly Added",
    "GC Unvisited",
    "GC Visited",
    "GC Root",
};

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

// TODO: rewrite the collected space code here!

