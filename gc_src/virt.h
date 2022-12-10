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

typedef struct {
    addr_table_code code;
    uint64_t index;
} addr_table_put_res;

// Attempt to put a physical address into the adt.
addr_table_put_res adt_put(addr_table *adt, void *paddr);

// Free a specific index in the table.
//
// NOTE: behavoir is undefined if the index is out of bounds
// or if the cell referenced is not in use. Doing these actions
// can corrupt the ADT forever.
//
addr_table_code adt_free(addr_table *adt, uint64_t index);

#endif
