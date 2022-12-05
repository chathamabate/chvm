#include "./virtual.h"
#include <pthread.h>
#include <sys/_pthread/_pthread_rwlock_t.h>
#include "../core_src/thread.h"
#include "mem.h"

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
    
    // Here are some variables used for looping over all
    // the entries.
    addr_table_entry *iter = (addr_table_entry *)(adt_h + 1);
    addr_table_entry *next = NULL;
    addr_table_entry *end = iter + cap;
    
    // Init our header.
    adt_h->free_list = iter; // First cell will start our free list.
    *(uint64_t *)&(adt_h->cap) = cap; // Hacky way of setting our constant.
    safe_rwlock_init(&(adt_h->free_list_lock), NULL);
    

    while (iter < end - 1) {
        iter->empty = 1;
        safe_rwlock_init(&(iter->lock), NULL);
        
        next = iter + 1;
        iter->next_free = next;
        iter = next;
    }

    // Finally, we've reached the last cell.
    iter->empty = 1;
    safe_rwlock_init(&(iter->lock), NULL);
    iter->next_free = NULL;

    // The last cell will mark the end of the initial
    // free list.

    return adt;
}

uint64_t adt_cap(addr_table *adt) {
    // No need for locking as cap should never
    // be changed.
    return ((addr_table_header *)adt)->cap;
}

uint64_t adt_put(addr_table *adt, void *paddr) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    addr_table_entry *table_start = (addr_table_entry *)(adt_h + 1);

    // We are going to be editing the free list,
    // so we need the write lock.
    safe_wrlock(&(adt_h->free_list_lock));

    addr_table_entry *free_cell = adt_h->free_list;

    // I don't believe we need to do any work with
    // the lock on the free cell itself.
    // As long as the user uses this datastructure correctly,
    // if the cell is in the free list, it will
    // never be accessed outside of this function. 
    
    adt_h->free_list = free_cell->next_free;

    // We are assuming that the user will only start to
    // use the index of this cell after this function
    // returns. Thus, we should not need to lock on
    // the cell.

    free_cell->empty = 0;
    free_cell->paddr = paddr;

    safe_rwlock_unlock(&(adt_h->free_list_lock));

    return (uint64_t)((void *)free_cell - (void *)table_start) / 
        sizeof(addr_table_entry);
}

void adt_set(addr_table *adt, uint64_t ind, void *paddr) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    addr_table_entry *entry = (addr_table_entry *)(adt_h + 1) + ind;
    
    safe_wrlock(&(entry->lock));
    entry->paddr = paddr;
    safe_rwlock_unlock(&(entry->lock));
}

void *adt_get_read(addr_table *adt, uint64_t ind) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    addr_table_entry *entry = (addr_table_entry *)(adt_h + 1) + ind;

    safe_rdlock(&(entry->lock));
    return entry->paddr;
}

void *adt_get_write(addr_table *adt, uint64_t ind) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    addr_table_entry *entry = (addr_table_entry *)(adt_h + 1) + ind;

    safe_wrlock(&(entry->lock));
    return entry->paddr;
}

void adt_unlock(addr_table *adt, uint64_t ind) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    addr_table_entry *entry = (addr_table_entry *)(adt_h + 1) + ind;

    safe_rwlock_unlock(&(entry->lock));
}


void adt_free(addr_table *adt, uint64_t ind) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    addr_table_entry *entry = (addr_table_entry *)(adt_h + 1) + ind;

    // NOTE: we are acquiring the write lock to be safe.
    // Although, in theory, a free call should never be called
    // in parallel with any other type of call on a cell.
    //
    // If we are calling free, we are assuming our object
    // isn't refernced or used by anyone anymore.
    safe_wrlock(&(entry->lock));

    entry->empty = 1;

    // The ordering here should never result in deadlock
    // as never does a thread acquire the free list lock
    // and then try to acquire a cell lock.
    safe_wrlock(&(adt_h->free_list_lock));

    // Add entry to the free list.
    entry->next_free = adt_h->free_list; 
    adt_h->free_list = entry;

    safe_rwlock_unlock(&(adt_h->free_list_lock));
    safe_rwlock_unlock(&(entry->lock));
}

#define NULL_INDEX UINT64_MAX

typedef struct addr_book_entry_struct {
    // This field will only be used when a table is in
    // the incomplete table list of the book.
    uint64_t next;

    addr_table *adt;
} addr_book_entry;

// Basically an array list of addr_book_entries with
// special functionality.
typedef struct addr_book_struct {
    // Capcity of each table in the book.
    const uint64_t table_cap;

    // NOTE:
    // This is the will look like a free list of
    // tables, however it is slightly different.
    // This list will link every table which has room
    // for at least one more physicall address.
    //
    // NOTE: we must use indeces for this list, as
    // arr is often realloced and moved around.
    uint64_t incomplete_tables;

    // Since arr will grow, we cannot have it be built
    // into this struct.
    uint64_t len;
    uint64_t cap;
    addr_book_entry *arr;

    // The lock will enforce sequential puts and frees to
    // the book.
    pthread_rwlock_t put_lock;
} addr_book;

addr_book *new_addr_book(uint8_t chnl, 
        uint64_t init_cap, uint64_t table_cap) {
    addr_book *adb = safe_malloc(chnl, sizeof(addr_book));
    
    *(uint64_t *)&(adb->table_cap) = table_cap;

    // No incomplete tables yet.
    adb->incomplete_tables = NULL_INDEX;
    adb->len = 0;
    adb->cap = init_cap;
    adb->arr = safe_malloc(chnl, sizeof(addr_book_entry) * init_cap);
    safe_rwlock_init(&(adb->put_lock), NULL);

    return adb;
}

void delete_addr_book(addr_book *adb) {
    // Again, No thread safety here...
    // If this is parallel with some other call...
    // The user fucked up.
    
    uint64_t i;
    for (i = 0; i < adb->len; i++) {
        delete_addr_table(adb->arr[i].adt);
    }

    safe_free(adb->arr);
    safe_free(adb);
}

addr_book_lookup adb_put(addr_book *adb, void *paddr) {
    safe_wrlock(&(adb->put_lock));

    if (adb->incomplete_tables == NULL_INDEX) { // i.e. there are no incomplete tables.
        // Here the are no available entries in any current
        // tables. We must add a new table (and maybe even resize)
        
        // Check to see if we must expand.
        if (adb->len == adb->cap) {
            adb->cap *= 2;
            adb->arr = safe_realloc(adb->arr, sizeof(addr_book_entry) *adb->cap);
        }

        // regardless if we expand, we will still be writing
        // to the table array, so write lock is justified.
        
        addr_book_entry *new_entry = adb->arr + adb->len++;

        // Create new entry.
        new_entry->next = NULL_INDEX;
        new_entry->adt = new_addr_table(get_chnl(adb), adb->table_cap);

        // Add entry to the incomplete list.
        adb->incomplete_tables = adb->len - 1;
    }

    // Here we know with certainty there is at least 1 incomplete table
    // in arr. 
    // Now we will need to read from and write to arr to prep our book entry.
    
    // Get the table to add to.
    addr_book_entry *entry = adb->arr + adb->incomplete_tables;

    uint64_t table = adb->incomplete_tables; // Convert back to zero index.
    uint64_t index = adt_put(entry->adt, paddr);

    addr_book_lookup lookup = {
        .table = table,
        .index = index,
    };

    // Finally, see if the table we just added to is no longer
    // incomplete.
    
    addr_table_header *adt_h = (addr_table_header *)(entry->adt);

    // NOTE: this is noted below too...
    // We are assuming the user only has access to the address book,
    // not the underlying adts. Thus, since this call is atomic on adb,
    // we don't need to worry about locking for the adt.
    if (((addr_table_header *)entry->adt)->free_list == NULL) {
        // Remove it from the incomplete tables list.
        adb->incomplete_tables = entry->next;
        entry->next = 0; // Not totally needed but whatevs.
    }

    safe_rwlock_unlock(&(adb->put_lock));
    return lookup;
}

static inline addr_table *adb_get_adt(addr_book *adb, uint64_t table) {
    addr_table *adt;

    // NOTE : this works since addresss of address tables
    // are always constant. Even if the address book gets
    // realloced or moved around, adt below will always be
    // correct.
    
    safe_rdlock(&(adb->put_lock));
    adt = adb->arr[table].adt;
    safe_rwlock_unlock(&(adb->put_lock));

    return adt;
}

void adb_set(addr_book *adb, addr_book_lookup vaddr, 
        void *paddr) {
    adt_set(adb_get_adt(adb, vaddr.table), vaddr.index, paddr);
} 

void *adb_get_read(addr_book *adb, addr_book_lookup vaddr) {
    return adt_get_read(adb_get_adt(adb, vaddr.table), vaddr.index);
}

void *adb_get_write(addr_book *adb, addr_book_lookup vaddr) {
    return adt_get_write(adb_get_adt(adb, vaddr.table), vaddr.index);
}

void adb_unlock(addr_book *adb, addr_book_lookup vaddr) {
    adt_unlock(adb_get_adt(adb, vaddr.table), vaddr.index);
}

void adb_free(addr_book *adb, addr_book_lookup vaddr) {
    // NOTE: For this implementation we are assumeing that
    // the user does not have access ot the individual adts held
    // in this address book.
    //
    // Thus, because this call is atomic on the address book,
    // we do not need to worry about thread safety for the adts.
    
    safe_wrlock(&(adb->put_lock));
    
    addr_book_entry *entry = adb->arr + vaddr.table;    
    addr_table_header *adt_h = (addr_table_header *)(entry->adt);
    
    uint8_t newly_incomplete = adt_h->free_list == NULL;

    adt_free(entry->adt, vaddr.index);

    // If we have freed a slot in a previously full
    // table, add the table to the incomplete list!
    if (newly_incomplete) {
        entry->next = adb->incomplete_tables;
        adb->incomplete_tables = vaddr.table;
    }

    safe_rwlock_unlock(&(adb->put_lock));
}

