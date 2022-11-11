#ifndef CORE_DATA_H
#define CORE_DATA_H

#include <stdlib.h>

// This file contains data defintions that
// are core to the entire system.
// i.e. are requited for the testing framework...
// thus cannot be tested themselves.

typedef struct {
    // Size of each cell in the simple list.
    size_t cell_size;
    
    // Number of cells filled.
    uint64_t len;

    // Number of total cells.
    uint64_t cap;

    // Buffer, will always have size cap * cell_size.
    void *buf;
} slist; // Simple list.


slist *new_slist(uint8_t chnl, size_t cs);
void delete_slist(slist *sl);
void sl_add(slist *sl, void *buf);

#endif
