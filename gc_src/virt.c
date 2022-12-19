#include "./virt.h"
#include <pthread.h>
#include <stdint.h>
#include <sys/_pthread/_pthread_rwlock_t.h>
#include "../core_src/sys.h"
#include "../core_src/thread.h"
#include "../core_src/mem.h"
#include "../core_src/io.h"

// The structure of the address table in memory is going
// to be a bit funky.

typedef struct {
    // This needs no lock as it will alwyas be
    // constant.
    const uint64_t cap;

    // Mutex for working with the free stack.
    pthread_rwlock_t free_stack_lck;
    uint64_t stack_fill;
} addr_table_header;

// After the header there will be cap * uint64_t indeces.
// This will form a stack of free indeces in the table.

// after this will be the cells themselves.

typedef struct {
    pthread_rwlock_t lck; 
    void *paddr;
} addr_table_cell;

addr_table *new_addr_table(uint8_t chnl, uint64_t cap) {
    addr_table *adt = safe_malloc(chnl, 
            sizeof(addr_table_header) + 
            (sizeof(uint64_t) * cap) + 
            (sizeof(addr_table_cell) * cap)
    );

    addr_table_header *adt_h = (addr_table_header *)adt;
    uint64_t *free_stack = (uint64_t *)(adt_h + 1);
    addr_table_cell *table = (addr_table_cell *)(free_stack + cap);

    // Init the header.
    //
    // Hacky way of setting a constant.
    *(uint64_t *)&(adt_h->cap) = cap;
    safe_rwlock_init(&(adt_h->free_stack_lck), NULL);
    adt_h->stack_fill = cap; // Free stack starts full.

    // Init the free queue.
    // This will hold all cells at first.
    
    uint64_t i;
    for (i = 0; i < cap; i++) {
        free_stack[i] = i;
    }

    // Init each cell now.
    for (i = 0; i < cap; i++) {
        safe_rwlock_init(&(table[i].lck), NULL);
        table[i].paddr = NULL;  // Not necessary, but whatevs.
    }

    return adt;
}

void delete_addr_table(addr_table *adt) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    uint64_t *free_q = (uint64_t *)(adt_h + 1);
    addr_table_cell *table = (addr_table_cell *)(free_q + adt_h->cap);

    safe_rwlock_destroy(&(adt_h->free_stack_lck));

    uint64_t i;
    for (i = 0; i < adt_h->cap; i++) {
        safe_rwlock_destroy(&(table[i].lck));
    }

    safe_free(adt);
}

uint64_t adt_get_cap(addr_table *adt) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    return adt_h->cap;
}

uint64_t adt_get_fill(addr_table *adt) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    uint64_t fill;

    safe_rdlock(&(adt_h->free_stack_lck));
    fill = adt_h->cap - adt_h->stack_fill;
    safe_rwlock_unlock(&(adt_h->free_stack_lck));

    return fill;
}

addr_table_put_res adt_put(addr_table *adt, void *paddr) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    uint64_t *free_stack = (uint64_t *)(adt_h + 1);
    addr_table_cell *table = (addr_table_cell *)(free_stack + adt_h->cap);

    addr_table_put_res res;

    uint64_t free_ind;
    safe_wrlock(&(adt_h->free_stack_lck));

    // No free cells left.
    if (adt_h->stack_fill == 0) {
        res.code = ADT_NO_SPACE;
        safe_rwlock_unlock(&(adt_h->free_stack_lck));

        return res;
    }

    // Here there is a free index in the stack...
    // Pop it off my friend...
    free_ind = free_stack[--(adt_h->stack_fill)];

    res.code = adt_h->stack_fill == 0 
        ? ADT_NEWLY_FULL : ADT_SUCCESS;

    safe_rwlock_unlock(&(adt_h->free_stack_lck));

    // We have aquired our free index...
    // Now to write to it.
    safe_wrlock(&(table[free_ind].lck));
    table[free_ind].paddr = paddr;
    safe_rwlock_unlock(&(table[free_ind].lck));

    res.index = free_ind;

    return res;
}

// Get the physical address at ind.
// The read lock will be requested on the address.
void *adt_get_read(addr_table *adt, uint64_t ind) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    uint64_t *free_stack = (uint64_t *)(adt_h + 1);
    addr_table_cell *table = (addr_table_cell *)(free_stack + adt_h->cap);

    addr_table_cell *cell = table + ind;

    safe_rdlock(&(cell->lck));

    return cell->paddr;
}

// Same as adt_get_read, except with a write lock.
void *adt_get_write(addr_table *adt, uint64_t ind) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    uint64_t *free_stack = (uint64_t *)(adt_h + 1);
    addr_table_cell *table = (addr_table_cell *)(free_stack + adt_h->cap);
    
    addr_table_cell *cell = table + ind;

    safe_wrlock(&(cell->lck));

    return cell->paddr;
}

// Unlock the entry at index.  
void adt_unlock(addr_table *adt, uint64_t ind) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    uint64_t *free_stack = (uint64_t *)(adt_h + 1);
    addr_table_cell *table = (addr_table_cell *)(free_stack + adt_h->cap);

    addr_table_cell *cell = table + ind;

    safe_rwlock_unlock(&(cell->lck));
}

addr_table_code adt_free(addr_table *adt, uint64_t index) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    uint64_t *free_stack = (uint64_t *)(adt_h + 1);

    addr_table_code res_code;

    safe_wrlock(&(adt_h->free_stack_lck));

    // Check to see if the table was previously full.
    res_code = adt_h->stack_fill == 0 
        ? ADT_NEWLY_FREE : ADT_SUCCESS;

    free_stack[(adt_h->stack_fill)++] = index;

    safe_rwlock_unlock(&(adt_h->free_stack_lck));

    return res_code;
}

const addr_book_vaddr NULL_VADDR = {
    .cell_index = UINT64_MAX,
    .table_index = UINT64_MAX,
};

static const uint64_t ADB_NULL_INDEX = UINT64_MAX;

// Our address book will use a doubly linked free list design
// to allow for easy removal of adts which are not at the head or
// tail of the list.
typedef struct addr_book_entry_struct {
    // This field is not necessarily needed.
    // However, for clarity, it shall be kept.
    uint8_t in_free_list;

    // Pointers always with indeces! 
    uint64_t next; 
    uint64_t prev;

    addr_table *adt;
} addr_book_entry;

typedef struct addr_book_struct {
    const uint64_t table_cap;

    // NOTE: Unlike in adts, address books automatically 
    // resize themselves. 
    pthread_rwlock_t lck;

    uint64_t free_list;

    uint64_t book_len;
    uint64_t book_cap;
    addr_book_entry *book;
} addr_book;

addr_book *new_addr_book(uint8_t chnl, uint64_t table_cap) {
    addr_book *adb = safe_malloc(chnl, sizeof(addr_book));

    *(uint64_t *)&(adb->table_cap) = table_cap;

    safe_rwlock_init(&(adb->lck), NULL);

    adb->free_list = ADB_NULL_INDEX;

    adb->book_len = 0;

    // Allocate a single entry in the book.
    // Don't even initialize it yet though.
    // This will all be done during our first put.
    adb->book_cap = 1;
    adb->book = safe_malloc(chnl, sizeof(addr_book_entry) * adb->book_cap);

    return adb;
}

void delete_addr_book(addr_book *adb) {
    // This call should never be in parallel with some
    // other call to the adb... but lock just to be
    // safe.
    safe_wrlock(&(adb->lck));

    // Delete all tables in the book.
    uint64_t i;
    for (i = 0; i < adb->book_len; i++) {
        delete_addr_table(adb->book[i].adt);
    }

    if (adb->book) {
        safe_free(adb->book);
    }

    safe_rwlock_unlock(&(adb->lck));

    safe_free(adb);
}

// Push an entry onto the free list.
// NOTE: we must have the write lock before calling this function!
static inline void unsafe_adb_push_free_list(addr_book *adb, 
        uint64_t entry_index) {
    addr_book_entry *entry = &(adb->book[entry_index]);

    if (adb->free_list != ADB_NULL_INDEX) {
        adb->book[adb->free_list].prev = entry_index;
    }

    entry->prev = ADB_NULL_INDEX;
    entry->next = adb->free_list;

    adb->free_list = entry_index;

    entry->in_free_list = 1;
}

static inline void adb_try_expand(addr_book *adb) {
    safe_wrlock(&(adb->lck));

    // Only expand if we need to. (at the time of request)
    if (adb->free_list != ADB_NULL_INDEX) {
        safe_rwlock_unlock(&(adb->lck));
        return;
    } 

    // Expansion time!

    // Expand book if we need to.
    if (adb->book_len == adb->book_cap) {
        adb->book_cap *= 2;
        adb->book = safe_realloc(adb->book, 
                sizeof(addr_book_entry) * adb->book_cap);
    }

    uint64_t table_index = adb->book_len;
    addr_table *table = new_addr_table(get_chnl(adb), adb->table_cap);

    // Store our new table in the book!
    adb->book[table_index].adt = table;
    (adb->book_len)++;

    // Push our new table onto the free list!
    unsafe_adb_push_free_list(adb, table_index);

    safe_rwlock_unlock(&(adb->lck));
}

static inline void adb_try_removal(addr_book *adb, uint64_t entry_index) {
    safe_wrlock(&(adb->lck));
    addr_book_entry *entry = &(adb->book[entry_index]);

    // NOTE: When we expanded, we checked to see if an expansion
    // was needed, if it wasn't we exited without doing anything.
    //
    // Here we will do the same procedure, as another thread could've
    // already removed this table from the free list.
    //
    // Additionally, our table may no longer be full, thus not needing
    // a removal... To check this, we will need to lock on the ADT
    // itself, this is ok to do once in a while.
    
    if (!(entry->in_free_list)) {
        safe_rwlock_unlock(&(adb->lck));
        return;
    }


    // Ok, this is giving me a headache to think about, so I have
    // decided to take the safe route. We will read lock on the ADT
    // for the entire addition to the free list.
    // This way we know while it is being added, it does in fact have
    // space.
    
    addr_table_header *adt_h = (addr_table_header *)entry->adt; 
    safe_rdlock(&(adt_h->free_stack_lck));

    // Check if there is space. (If so, don't remove)
    if (adt_h->stack_fill > 0) {
        safe_rwlock_unlock(&(adt_h->free_stack_lck));
        safe_rwlock_unlock(&(adb->lck));
        return;
    }

    if (entry->prev != ADB_NULL_INDEX) {
        adb->book[entry->prev].next = entry->next;
    } else {
        // If our entryy is in the free list and doesn't 
        // have a prev pointer, it must be the head of the list.
        adb->free_list = entry->next;
    }

    if (entry->next != ADB_NULL_INDEX) {
        adb->book[entry->next].prev = entry->prev;
    }

    entry->prev = ADB_NULL_INDEX;
    entry->next = ADB_NULL_INDEX;

    entry->in_free_list = 0;

    safe_rwlock_unlock(&(adt_h->free_stack_lck));
    safe_rwlock_unlock(&(adb->lck));
}

addr_book_vaddr adb_put(addr_book *adb, void *paddr) {
    addr_book_vaddr vaddr;

    while (1) {
        // First we need a table to try to put to.
        uint64_t entry_index;
        
        safe_rdlock(&(adb->lck)); 
        entry_index = adb->free_list;
        safe_rwlock_unlock(&(adb->lck));

        // Here there was no table found at the time of search.
        // So, lets add a new one and try again.
        if (entry_index == ADB_NULL_INDEX) {
            adb_try_expand(adb);
            continue;
        }

        addr_table *adt;

        // Since adb's never shrink (as of now) we know our
        // entry index will always point to a valid ADT.
        safe_rdlock(&(adb->lck));
        adt = adb->book[entry_index].adt;
        safe_rwlock_unlock(&(adb->lck));

        // Don't need a lock or index for this one as ADT
        // addresses remain constant!
        addr_table_put_res put_res = adt_put(adt, paddr);

        // The entry we pulled ended up being full.
        // Let's just try again.
        if (put_res.code == ADT_NO_SPACE) {
            continue;
        }

        // This request filled our table, at this point in time,
        // it is possible are table is full and in the free list.
        // Let's try and do a removal.
        //
        // NOTE: it is also possible our table is no longer full, or
        // not even in the free list, try removal will account for
        // this!
        if (put_res.code == ADT_NEWLY_FULL) {
            adb_try_removal(adb, entry_index); 
        }

        // Regardless of whether a removal occurs,
        // we have enough to return!
        vaddr.table_index = entry_index;
        vaddr.cell_index = put_res.index;

        return vaddr;
    }
}

static inline addr_table *adb_get_adt(addr_book *adb, uint64_t table_index) {
    addr_table *adt;

    safe_rdlock(&(adb->lck));
    adt = adb->book[table_index].adt;
    safe_rwlock_unlock(&(adb->lck));

    return adt;
}

void *adb_get_read(addr_book *adb, addr_book_vaddr vaddr) {
    addr_table *adt = adb_get_adt(adb, vaddr.table_index);

    return adt_get_read(adt, vaddr.cell_index);
}

void *adb_get_write(addr_book *adb, addr_book_vaddr vaddr) {
    addr_table *adt = adb_get_adt(adb, vaddr.table_index);

    return adt_get_write(adt, vaddr.cell_index);
}

void adb_unlock(addr_book *adb, addr_book_vaddr vaddr) {
    addr_table *adt = adb_get_adt(adb, vaddr.table_index);

    adt_unlock(adt, vaddr.cell_index);
}

static inline void adb_try_addition(addr_book *adb, uint64_t entry_index) {
    safe_wrlock(&(adb->lck));
    addr_book_entry *entry = &(adb->book[entry_index]);

    if (entry->in_free_list) {
        safe_rwlock_unlock(&(adb->lck));
        return;
    }

    addr_table_header *adt_h = (addr_table_header *)entry->adt; 

    // Here our entry is not in the free list yet.
    // Make sure it is not full.
    safe_rdlock(&(adt_h->free_stack_lck));

    if (adt_h->stack_fill == 0) {
        safe_rwlock_unlock(&(adt_h->free_stack_lck));
        safe_rwlock_unlock(&(adb->lck));

        return;
    }
    
    // We have made it here.
    // Our table has space, but is not in the free list yet.
    // Time to add!
    unsafe_adb_push_free_list(adb, entry_index);

    safe_rwlock_unlock(&(adt_h->free_stack_lck));
    safe_rwlock_unlock(&(adb->lck));
}

void adb_free(addr_book *adb, addr_book_vaddr vaddr) {
    addr_table *adt = adb_get_adt(adb, vaddr.table_index);

    addr_table_code free_res = adt_free(adt, vaddr.cell_index);

    if (free_res == ADT_NEWLY_FREE) {
        adb_try_addition(adb, vaddr.table_index);
    }
}

uint64_t adb_get_fill(addr_book *adb) {
    uint64_t fill = 0;

    // This kinda does need to lock on everything.
    // Not really meant to be used in performance
    // critical areas.

    safe_rdlock(&(adb->lck));
    uint64_t i;
    for (i = 0; i < adb->book_len; i++) {
        addr_table *adt = adb->book[i].adt;
        fill += adt_get_fill(adt);
    }
    safe_rwlock_unlock(&(adb->lck));

    return fill;
}
