#ifndef GC_VIRUTAL_H
#define GC_VIRUTAL_H

#include <stdint.h>
#include "../core_src/mem.h"

// Basically a sub for a void *.
typedef struct {} addr_table;

addr_table *new_addr_table(uint8_t chnl, uint64_t cap);

static inline void delete_addr_table(addr_table *adt) {
    safe_free(adt);
}

// Get the capcity of the table (This is fixed)
uint64_t adt_cap(addr_table *adt);

// This puts a physical address into the address table.
// It returns the index of the new entry.
// Assumes there is room for this address,
// behavoir is undefined if there is not.
uint64_t adt_put(addr_table *adt, void *paddr);

// NOTE: in the below functions, there is no index checking.
// Be Careful, make sure your index is in bounds and
// points to a valid cell.

// Edit an already in use cell in the table.
void adt_set(addr_table *adt, uint64_t ind, void *paddr);

// Get the physical address at ind.
// The read lock will be requested on the address.
void *adt_get_read(addr_table *adt, uint64_t ind);

// Same as adt_get_read, except with a write lock.
void *adt_get_write(addr_table *adt, uint64_t ind);

// Unlock the entry at index.  
void adt_unlock(addr_table *adt, uint64_t ind);

// This takes an occupied cell and adds it to the
// free list. 
// If index is out of bounds, behavoir is undefined.
// NOTE: ind should not be used to access this cell
// after return. It is imperative that we only free a
// cell when all threads are done working with it.
void adt_free(addr_table *adt, uint64_t ind);

typedef struct addr_book_struct addr_book;

typedef struct {
    const uint64_t table; // kinda like a page in a book.
    const uint64_t index; // Kinda like the line number on a page.
} addr_book_lookup;

addr_book *new_addr_book(uint8_t chnl, 
        uint64_t init_cap, uint64_t table_cap);

void delete_addr_book(addr_book *adb);

// Add a physical address to the book, 
// get the virtual address back.
addr_book_lookup adb_put(addr_book *adb, void *paddr);

// Set an in use address.
void adb_set(addr_book *adb, addr_book_lookup vaddr, 
        void *paddr);

// Look up a virtual address, get the
// physical address back.
// Behavoir undefined if virtual address
// has not been initialized yet.
void *adb_get_read(addr_book *adb, addr_book_lookup vaddr);

void *adb_get_write(addr_book *adb, addr_book_lookup vaddr);

void adb_unlock(addr_book *adb, addr_book_lookup vaddr);

// Mark a specific virtual address as unused.
// Again, does no checking that vaddr is valid and in use.
void adb_free(addr_book *adb, addr_book_lookup vaddr);

#endif
