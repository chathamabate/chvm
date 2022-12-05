#include "./virtual.h"
#include <pthread.h>

typedef struct addr_table_entry_struct {
    // Lock for the table entry.
    pthread_rwlock_t lock;

    // Whether or not this cell is active.
    uint8_t empty;

    // If this is an empty cell, we will store a pointer
    // to the next empty cell in the table.
    // Otherwise, we will store the physical address
    // stored in the cell.
    union {
        void *paddr;
        struct addr_table_entry_struct *next_free;
    };
} addr_table_entry;

typedef struct {
    // The free list will link together cells which
    // are not in use.
    addr_table_entry *free_list;

    // This lock will be used for reading from and writing
    // to specifically the free list pointer above.
    // All other thread safety will be entry specific.
    pthread_rwlock_t free_list_lock;
    
    // How many entry will be contained in the 
    // table. THIS NEVER CHANGES.
    // Thus, no locks should be needed on it.
    const uint64_t cap; 
} addr_table_header;

addr_table *new_addr_table(uint8_t chnl, uint64_t cap) {
    addr_table *adt = safe_malloc(chnl, 
            sizeof(addr_table_header) + (cap * sizeof(addr_table_entry)));

    addr_table_header *adt_h = (addr_table_header *)adt;
    
    addr_table_entry *iter = (addr_table_entry *)(adt_h + 1);
    addr_table_entry *end = iter + cap;
    
    adt_h->free_list = iter;
    *(uint64_t *)&(adt_h->cap) = cap; // Hacky way of setting our constant.
    
    // TODO finish up table creation.

    for (; iter < end; iter++) {
    }

    // The last cell will mark the end of the initial
    // free list.

    return adt;
}

uint64_t adt_cap(addr_table *adt) {
    return ((addr_table_header *)adt)->cap;
}

uint8_t adt_has_next(addr_table *adt) {
    return ((addr_table_header *)adt)->free_list != NULL;
}

uint64_t adt_put(addr_table *adt, void *paddr) {
    addr_table_header *adt_h = (addr_table_header *)adt;

    // Get the next free cell.
    void **free_cell = adt_h->free_list;

    // Remove it from the free list.
    adt_h->free_list = *free_cell;

    // Place the physical address into the free cell.
    *free_cell = paddr;

    void *table_start = (void *)(adt_h + 1);

    return (uint64_t)((void *)free_cell - table_start) / sizeof(void *);
}

void adt_set(addr_table *adt, uint64_t ind, void *paddr) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    void **table_start = (void *)(adt_h + 1);

    table_start[ind] = paddr;
}

void *adt_get(addr_table *adt, uint64_t ind) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    void **table_start = (void *)(adt_h + 1);

    return table_start[ind];
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

// Basically an array list of addr_entries *s with
// special functionality.
typedef struct addr_book_struct {
    uint64_t len;
    uint64_t cap;

    // Capcity of each table in the book.
    uint64_t table_cap;

    // NOTE:
    // This is the will look like a free list of
    // tables, however it is slightly different.
    // This list will link every table which has room
    // for at least one more physicall address.
    addr_book_entry *incomplete_tables;

    // Since arr will grow, we cannot have it be built
    // into this struct.
    addr_book_entry *arr;
} addr_book;

addr_book *new_addr_book(uint8_t chnl, 
        uint64_t init_cap, uint64_t table_cap) {
    addr_book *adb = safe_malloc(chnl, sizeof(addr_book));
    
    adb->cap = init_cap;
    adb->table_cap = table_cap;
    adb->incomplete_tables = NULL; // No tables allocated yet.
    adb->len = 0;

    adb->arr = safe_malloc(chnl, sizeof(addr_book_entry) * init_cap);

    return adb;
}

void delete_addr_book(addr_book *adb) {
    uint64_t i;
    for (i = 0; i < adb->len; i++) {
        delete_addr_table(adb->arr[i].adt);
    }

    safe_free(adb->arr);
    safe_free(adb);
}

addr_book_lookup adb_put(addr_book *adb, void *paddr) {
    if (adb->incomplete_tables == NULL) {
        // Here the are no available entries in any current
        // tables. We must add a new table.

        // Check to see if we must expand.
        if (adb->len == adb->cap) {
            adb->cap *= 2;
            adb->arr = safe_realloc(adb->arr, sizeof(addr_book_entry) *adb->cap);
        }
        
        addr_book_entry *new_entry = adb->arr + adb->len++;

        // Create new entry.
        new_entry->next = NULL;
        new_entry->adt = new_addr_table(get_chnl(adb), adb->table_cap);

        // Add entry to the incomplete list.
        adb->incomplete_tables = new_entry;
    }
    
    // Get the table to add to.
    addr_book_entry *entry = adb->incomplete_tables;
    uint64_t table = (uint64_t)((void *)entry - (void *)(adb->arr)) / sizeof(addr_book_entry);
    uint64_t index = adt_put(entry->adt, paddr);

    addr_book_lookup lookup = {
        .table = table,
        .index = index,
    };

    // Finally, see if the table we just added to is no longer
    // incomplete.

    if (adt_is_full(entry->adt)) {
        adb->incomplete_tables = entry->next;
        entry->next = NULL; // Not totally needed but whatevs.
    }

    return lookup;
}

void adb_set(addr_book *adb, addr_book_lookup vaddr, 
        void *paddr) {
    adt_set(adb->arr[vaddr.table].adt, vaddr.index, paddr);
} 

void *adb_get(addr_book *adb, addr_book_lookup vaddr) {
    return adt_get(adb->arr[vaddr.table].adt, vaddr.index);
}

void adb_free(addr_book *adb, addr_book_lookup vaddr) {
    addr_book_entry *entry = adb->arr + vaddr.table;    
    
    uint8_t newly_incomplete = adt_is_full(entry->adt); 

    adt_free(entry->adt, vaddr.index);

    // If we have freed a slot in a previously full
    // table, add the table to the incomplete list!
    if (newly_incomplete) {
        entry->next = adb->incomplete_tables;
        adb->incomplete_tables = entry;
    }
}

