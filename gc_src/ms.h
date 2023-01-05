#ifndef GC_MS_H
#define GC_MS_H

#include "./virt.h"

typedef struct mem_space_struct mem_space;

mem_space *new_mem_space(uint8_t chnl, uint64_t adt_cap, uint64_t mb_m_bytes);
void delete_mem_space(mem_space *ms);

addr_book_vaddr ms_malloc(mem_space *ms, uint64_t min_bytes);
void ms_free(mem_space *ms, addr_book_vaddr vaddr);

#endif
