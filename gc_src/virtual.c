#include "./virtual.h"


typedef struct {
    void **free_list;

    // Maybe some thread safe lock at some point...

    // How many addresses will be contained in the 
    // table.
    uint64_t len; 
} addr_table_header;

addr_table *new_addr_table(uint8_t chnl, uint64_t len) {
    addr_table *adt = safe_malloc(chnl, 
            sizeof(addr_table_header) + (len * sizeof(void *)));

    addr_table_header *adt_h = (addr_table_header *)adt;

    void **iter = (void **)(adt_h + 1);
    
    adt_h->free_list = iter;
    adt_h->len = len;

    // This is the table section.
    // Each entry in the table will either contain a pointer
    // to some place in memory, or a pointer to the next free
    // cell in the table.
    // 
    // Upon creation, the table will contain only free cells.

    uint64_t c;
    for (c = 0; c < len -1; c++, iter++) {
        *iter = iter + 1;
    }

    // The last cell will mark the end of the initial
    // free list.
    *iter = NULL;

    return adt;
}

uint8_t adt_has_next(addr_table *adt) {
    return ((addr_table_header *)adt)->free_list != NULL;
}

uint64_t adt_next(addr_table *adt) {
    addr_table_header *adt_h = (addr_table_header *)adt;

    void **free_cell = adt_h->free_list;
    adt_h->free_list = *free_cell;

    void *table_start = (void *)(adt_h + 1);

    return (uint64_t)((void *)free_cell - table_start);
}

void adt_free(addr_table *adt, uint64_t ind) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    void **table_start = (void *)(adt_h + 1);

    // Push free cell onto front of the free list.
    table_start[ind] = adt_h->free_list;
    adt_h->free_list = table_start + ind; 
}

 
