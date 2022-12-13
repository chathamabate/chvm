#ifndef GC_VIRT_H
#define GC_VIRT_H

#include <stdint.h>

typedef struct {} addr_table;

// The codes will give hints to the user about the state
// of the ADT at the time an operation occurs.
typedef enum {
    // Everything happened as normal, nothing special.
    ADT_SUCCESS = 0,

    // When you try to add, but there is no space.
    ADT_NO_SPACE,

    // When after your action, the ADT is completely full.
    ADT_NEWLY_FULL,

    // When your action changes the ADT from full to not full.
    ADT_NEWLY_FREE, 
} addr_table_code;

// Cap will be the number of entries inn the table.
//
// NOTE: address tables never ever regrow.
// All entries must reside in a constant location in
// memory.
//
addr_table *new_addr_table(uint8_t chnl, uint64_t cap);

void delete_addr_table(addr_table *adt);

uint64_t adt_get_cap(addr_table *adt);

typedef struct {
    addr_table_code code;
    uint64_t index;
} addr_table_put_res;

// Attempt to put a physical address into the adt.
addr_table_put_res adt_put(addr_table *adt, void *paddr);

// Get the physical address at ind.
// The read lock will be requested on the address.
void *adt_get_read(addr_table *adt, uint64_t ind);

// Same as adt_get_read, except with a write lock.
void *adt_get_write(addr_table *adt, uint64_t ind);

// Unlock the entry at index.  
void adt_unlock(addr_table *adt, uint64_t ind);

// Free a specific index in the table.
//
// NOTE: behavoir is undefined if the index is out of bounds
// or if the cell referenced is not in use. Doing these actions
// can corrupt the ADT forever.
//
addr_table_code adt_free(addr_table *adt, uint64_t index);

typedef struct addr_book_struct addr_book;

typedef struct {
    const uint64_t table_index;
    const uint64_t cell_index;
} addr_book_vaddr;

addr_book *new_addr_book(uint8_t chnl, uint64_t table_cap);
void delete_addr_book(addr_book *adb);

addr_book_vaddr adb_put(addr_book *adb, void *paddr);

void *adb_get_read(addr_book *adb, addr_book_vaddr vaddr);

void *adb_get_write(addr_book *adb, addr_book_vaddr vaddr);

void adb_unlock(addr_book *adb, addr_book_vaddr vaddr);

void adb_free(addr_book *adb, addr_book_vaddr vaddr);


#endif
