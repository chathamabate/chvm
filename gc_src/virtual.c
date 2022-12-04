#include "./virtual.h"


typedef struct {
    void **free_list;

    // Add thread safety at some point.

    // How many addresses will be contained in the 
    // table. 
    uint64_t cap; 
} addr_table_header;

addr_table *new_addr_table(uint8_t chnl, uint64_t cap) {
    addr_table *adt = safe_malloc(chnl, 
            sizeof(addr_table_header) + (cap * sizeof(void *)));

    addr_table_header *adt_h = (addr_table_header *)adt;

    void **iter = (void **)(adt_h + 1);
    
    adt_h->free_list = iter;
    adt_h->cap = cap;

    // This is the table section.
    // Each entry in the table will either contain a pointer
    // to some place in memory, or a pointer to the next free
    // cell in the table.
    // 
    // Upon creation, the table will contain only free cells.

    uint64_t c;
    for (c = 0; c < cap - 1; c++, iter++) {
        *iter = iter + 1;
    }

    // The last cell will mark the end of the initial
    // free list.
    *iter = NULL;

    return adt;
}

uint64_t adt_cap(addr_table *adt) {
    return ((addr_table_header *)adt)->cap;
}

uint8_t adt_has_next(addr_table *adt) {
    return ((addr_table_header *)adt)->free_list != NULL;
}

uint64_t adt_next(addr_table *adt) {
    addr_table_header *adt_h = (addr_table_header *)adt;

    void **free_cell = adt_h->free_list;
    adt_h->free_list = *free_cell;

    void *table_start = (void *)(adt_h + 1);

    // Same as dividing by 8 here. (Converting from address to index.)
    return (uint64_t)((void *)free_cell - table_start) >> 3;
}

void adt_free(addr_table *adt, uint64_t ind) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    void **table_start = (void *)(adt_h + 1);

    // Push free cell onto front of the free list.
    table_start[ind] = adt_h->free_list;
    adt_h->free_list = table_start + ind; 
}

typedef struct addr_book_entry_struct {
    // This field will only be used whn a table is in
    // the incomplete table list of the book.
    struct addr_book_entry_struct *next;

    addr_table *adt;
} addr_book_entry;

// Basically an array list of addr_tables *s with
// special functionality.
typedef struct {
    uint64_t len;
    uint64_t cap;
    uint64_t table_cap;

    // NOTE:
    // This is the will look like a free list of
    // tables, however it is slightly different.
    // This list will link every table which has room
    // for at least one more physicall address.
    addr_book_entry *incomplete_tables;
} addr_book_header;

addr_book *new_addr_book(uint8_t chnl, 
        uint64_t init_cap, uint64_t table_cap) {
    addr_book *adb = safe_malloc(chnl, 
            sizeof(addr_book_header) + (init_cap * sizeof(addr_book_entry)));

    addr_book_header *adb_h = (addr_book_header *)adb;
    
    // Create header.
    adb_h->cap = init_cap;
    adb_h->table_cap = table_cap;
    adb_h->incomplete_tables = NULL; // No tables allocated yet.
    adb_h->len = 0;

    // Leave body of address book uninitializied.

    return adb;
}

void delete_addr_book(addr_book *adb) {
    addr_book_header *adb_h = (addr_book_header *)adb;
    addr_book_entry *adb_entries = (addr_book_entry *)(adb_h + 1);

    uint64_t i;
    for (i = 0; i < adb_h->len; i++) {
        delete_addr_table(adb_entries[i].adt);
    }

    safe_free(adb);
}


