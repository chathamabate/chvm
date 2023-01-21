#ifndef GC_MS_H
#define GC_MS_H

#include <time.h>
#include "./virt.h"

typedef struct mem_space_struct mem_space;

// Mem spaces will have their own address books!
mem_space *new_mem_space_seed(uint64_t chnl, uint64_t seed, 
        uint64_t adb_t_cap, uint64_t mb_m_bytes);

static inline mem_space *new_mem_space(uint64_t chnl, uint64_t adb_t_cap, 
        uint64_t mb_m_bytes) {
    return  new_mem_space_seed(chnl, time(NULL), adb_t_cap, mb_m_bytes);
}

void delete_mem_space(mem_space *ms);

addr_book_vaddr ms_malloc(mem_space *ms, uint64_t min_bytes);
void ms_free(mem_space *ms, addr_book_vaddr vaddr);

addr_book *ms_get_adb(mem_space *ms);

void *ms_get_write(mem_space *ms, addr_book_vaddr vaddr);
void *ms_get_read(mem_space *ms,addr_book_vaddr vaddr);
void ms_unlock(mem_space *ms,addr_book_vaddr vaddr);

#endif
