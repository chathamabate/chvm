#ifndef GC_MEM_H
#define GC_MEM_H

#include <stdint.h>
#include "./virt.h"

typedef struct {} mem_page;

// NOTE: the operations on the memory page itself will
// all be thread safe (i.e. the functions below)
// However, they are still open to user error if used
// incorrectly.

mem_page *new_mem_page(uint8_t chnl, uint64_t min_bytes);
void delete_mem_page(mem_page *mp);

// The size of the largest number of consecutive bytes which can
// be allocated in this block. 
uint64_t mp_get_space(mem_page *mp);

// Will return NULL if there isn't enough space at the time
// of request.
void *mp_malloc(mem_page *mp, uint64_t min_bytes);

void mp_free(mem_page *mp, void *ptr);






// NOTE: EVERYTHING BELOW IS FOR LATER!!!!!

// Here we will create the API for a garbage collected
// memory space.....

// A data block will be a variable size block of memory
// allocatable by our memory space.
typedef struct {} data_block;

uint64_t db_get_refs_len(data_block *db);
addr_book_vaddr *db_get_refs(data_block *db);

uint64_t db_get_data_len(data_block *db);
uint8_t *db_get_data(data_block *db);

// Ok...

typedef struct memory_space_struct memory_space;

memory_space *new_memory_space();
void delete_memory_space(memory_space *ms);

#endif
