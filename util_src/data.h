#ifndef UTIL_DATA_H
#define UTIL_DATA_H

#include <stddef.h>
#include <stdlib.h>

// Returns a positive value if both values are equal, otherwise 0.
typedef uint8_t (*equator)(void *val_ptr1, void *val_ptr2);

typedef struct util_linked_list util_ll;

util_ll *new_linked_list(uint8_t chnl, size_t cs);
void delete_linked_list(util_ll *ll);

void *ll_next(util_ll *ll);
void ll_add(util_ll *ll, void *src);

void *ll_get(util_ll *ll, uint64_t i);

uint64_t ll_len(util_ll *ll);

// Can be used as a stack or queue.
typedef struct util_broken_collection util_bc;

// cs = size of each cell.
// ts = number of cells in each table.
// del_empty_tables = whether or not the bc should free tables when they are empty...
// or just leave them be. (For a stack you may want to leave them, for a queue
// you will want to delete)
util_bc *new_broken_collection(uint8_t chnl, size_t cs, uint64_t ts, 
        uint8_t del_empty_tables);

void delete_broken_collection(util_bc *bc);

uint64_t bc_get_num_tables(util_bc *bc);

uint8_t bc_empty(util_bc *bc);

void bc_push_back(util_bc *bc, const void *src);
void bc_push_front(util_bc *bc, const void *src);

void bc_pop_back(util_bc *bc, void *dest);
void bc_pop_front(util_bc *bc, void *dest);

typedef void (*cell_consumer)(void *cell, void *ctx);

void bc_foreach(util_bc *bc, cell_consumer c, void *ctx);

uint64_t bc_len(util_bc *bc);

#endif
