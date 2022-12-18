#ifndef GC_MEM_H
#define GC_MEM_H

#include <stdint.h>
#include "./virt.h"

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
