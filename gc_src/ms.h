#ifndef GC_MS_H
#define GC_MS_H

#include <time.h>
#include "./virt.h"
#include "./mb.h"

typedef struct mem_space_struct mem_space;

// Mem spaces will have their own address books!
mem_space *new_mem_space_seed(uint64_t chnl, uint64_t seed, 
        uint64_t adb_t_cap, uint64_t mb_m_bytes);

static inline mem_space *new_mem_space(uint64_t chnl, uint64_t adb_t_cap, 
        uint64_t mb_m_bytes) {
    return  new_mem_space_seed(chnl, time(NULL), adb_t_cap, mb_m_bytes);
}

void delete_mem_space(mem_space *ms);

// This is something the users will see.
typedef struct {
    mem_block *mb;
} mem_space_malloc_header;

malloc_res ms_malloc_p(mem_space *ms, uint64_t min_bytes, uint8_t hold);

static inline addr_book_vaddr ms_malloc(mem_space *ms, uint64_t min_bytes) {
    return ms_malloc_p(ms, min_bytes, 0).vaddr;
}

static inline malloc_res ms_malloc_and_hold(mem_space *ms, 
        uint64_t min_bytes) {
    return ms_malloc_p(ms, min_bytes, 1);
}

void ms_free(mem_space *ms, addr_book_vaddr vaddr);

uint8_t ms_allocated(mem_space *ms, addr_book_vaddr vaddr);

// This will call try full shift on all memory blocks
// in the mem space at the time of the call.
void ms_try_full_shift(mem_space *ms);

void *ms_get_write(mem_space *ms, addr_book_vaddr vaddr);
void *ms_get_read(mem_space *ms, addr_book_vaddr vaddr);
void ms_unlock(mem_space *ms, addr_book_vaddr vaddr);

// NOTE:  While the below calls all are in a way "thread safe",
// it doesn't really make sense to call them in parallel with any other
// call.

void ms_foreach(mem_space *ms, mp_consumer c, void *ctx, uint8_t wr);
void ms_filter(mem_space *ms, mp_predicate pred, void *ctx);
uint64_t ms_count(mem_space *ms);

void ms_print(mem_space *ms);

#endif
