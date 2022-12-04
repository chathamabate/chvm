#ifndef GC_VIRUTAL_H
#define GC_VIRUTAL_H

#include <stdint.h>
#include "../core_src/mem.h"

// Basically a sub for a void *.
typedef struct {} addr_table;

addr_table *new_addr_table(uint8_t chnl, uint64_t len);

static inline void delete_addr_table(addr_table *adt) {
    safe_free(adt);
}

// Get the capcity of the table (This is fixed)
uint64_t adt_cap(addr_table *adt);

// Whether or not this table has any more free entries.
uint8_t adt_has_next(addr_table *adt);

static inline uint8_t adt_is_full(addr_table *adt) {
    return !adt_has_next(adt);
}

// This returns the index of the next free cell 
// in the table. The free cell chosen is removed from 
// the free list.
// If there are no free cells, behavoir is undefined.
uint64_t adt_next(addr_table *adt);

// This takes an occupied cell and adds it to the
// free list. 
// If index is out of bounds, behavoir is undefined.
void adt_free(addr_table *adt, uint64_t ind);

typedef struct {} addr_book;

typedef struct {
    uint64_t table; // kinda like a page in a book.
    uint64_t index; // Kinda like the line number on a page.
} addr_book_lookup;

addr_book *new_addr_book(uint8_t chnl, 
        uint64_t init_cap, uint64_t table_cap);

void delete_addr_book(addr_book *adb);

#endif
