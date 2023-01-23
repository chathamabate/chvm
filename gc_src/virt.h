#ifndef GC_VIRT_H
#define GC_VIRT_H

#include <stdint.h>

typedef struct {} addr_table;

// NOTE: These address book definitions were below, but since
// have moved since address table must be aware of address
// book vaddrs.
typedef struct addr_book_struct addr_book;

typedef struct {
    uint64_t table_index;
    uint64_t cell_index;
} addr_book_vaddr;

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

uint64_t adt_get_fill(addr_table *adt);

typedef struct {
    addr_table_code code;
    uint64_t index;
} addr_table_put_res;

// Attempt to put a physical address into the adt.
// If hold is specified, the lock on the paddr will be held after
// this function returns.
addr_table_put_res adt_put_p(addr_table *adt, void *paddr, uint8_t hold);

// Attempt to put a physical address into the adt.
static inline addr_table_put_res adt_put(addr_table *adt, void *paddr) {
    return adt_put_p(adt, paddr, 0);
}

static inline addr_table_put_res adt_put_and_hold(addr_table *adt, void *paddr) {
    return adt_put_p(adt, paddr, 1);
}

// NOTE: For safety, I have added an allocation flag to each
// cell in the address table.
// Thus, the following calls will be able to check if
// given indeces are valid at the time of call.
//
// If bad arguments are given, we just exit and print a message.

// This Copies data stored at a given cell's physical address
// to a new physical address. The new physical address in then
// placed in the adt at cell_ind.
//
// If init is 1, the cell's full vaddr will be placed
// just before new_paddr. If init is 0, table ind is
// ignored.
//
// NOTE: it is ok if the new area of memory overlaps with
// the old area of memory.
//
// I am ashamed to add a lock flag here, as it allows for unsafe 
// operations... This is a sacrifice I must take sadly.
void adt_move_p(uint8_t lck, addr_table *adt, uint64_t cell_ind, 
        void *new_paddr, uint64_t size, 
        uint8_t init, uint64_t table_ind);

static inline void adt_move(addr_table *adt, uint64_t cell_ind,
        void *new_paddr, uint64_t size) {
    adt_move_p(1, adt, cell_ind, new_paddr, size, 0, 0);
}

static inline void adt_reinstall(addr_table *adt, addr_book_vaddr vaddr,
        void *new_paddr, uint64_t size) {
    adt_move_p(1, adt, vaddr.cell_index, new_paddr, 
            size, 1, vaddr.table_index);
}


// Returns NULL if the lock wasn't acquired
void *adt_get_read_p(addr_table *adt, uint64_t ind, uint8_t blk);

static inline void *adt_get_read(addr_table *adt, uint64_t ind) {
    return adt_get_read_p(adt, ind, 1);
}

static inline void *adt_try_get_read(addr_table *adt, uint64_t ind) {
    return adt_get_read_p(adt, ind, 0);
}

void *adt_get_write_p(addr_table *adt, uint64_t ind, uint8_t blk);

// Same as adt_get_read, except with a write lock.
static inline void *adt_get_write(addr_table *adt, uint64_t ind) {
    return adt_get_write_p(adt, ind, 1);
}

static inline void *adt_try_get_write(addr_table *adt, uint64_t ind) {
    return adt_get_write_p(adt, ind, 0);
}

// Unlock the entry at index.  
void adt_unlock(addr_table *adt, uint64_t ind);

// Free a specific index in the table.
//
// NOTE: If this is called while the user has the lock on
// the given index acquired, behavoir is undefined.
addr_table_code adt_free(addr_table *adt, uint64_t index);

void adt_print_p(addr_table *adt, const char *prefix);

static inline void adt_print(addr_table *adt) {
    adt_print_p(adt, "");
}

// Address Book Signatures....

extern const addr_book_vaddr NULL_VADDR;

static inline uint8_t eq_adb_addr(addr_book_vaddr v1, 
        addr_book_vaddr v2) {
    return (v1.table_index == v2.table_index) && 
        (v1.cell_index == v2.cell_index);
}

static inline uint8_t null_adb_addr(addr_book_vaddr v) {
    return v.table_index == NULL_VADDR.table_index &&
        v.cell_index == NULL_VADDR.cell_index;
}

addr_book *new_addr_book(uint8_t chnl, uint64_t table_cap);
void delete_addr_book(addr_book *adb);

addr_book_vaddr adb_put_p(addr_book *adb, void *paddr, uint8_t hold);

static inline addr_book_vaddr adb_put(addr_book *adb, void *paddr) {
    return adb_put_p(adb, paddr, 0);
}

static inline addr_book_vaddr adb_put_and_hold(addr_book *adb, void *paddr) {
    return adb_put_p(adb, paddr, 1);
}

void adb_move_p(uint8_t lck, addr_book *adb, addr_book_vaddr vaddr, 
        void *new_paddr, uint64_t size, uint8_t init);

static inline void adb_move(addr_book *adb, addr_book_vaddr vaddr, 
        void *new_paddr, uint64_t size) {
    adb_move_p(1, adb, vaddr, new_paddr, size, 0);
}

static inline void adb_reinstall(addr_book *adb, addr_book_vaddr vaddr, 
        void *new_paddr, uint64_t size) {
    adb_move_p(1, adb, vaddr, new_paddr, size, 1);
}

void *adb_get_read(addr_book *adb, addr_book_vaddr vaddr);

void *adb_try_get_read(addr_book *adb, addr_book_vaddr vaddr);

void *adb_get_write(addr_book *adb, addr_book_vaddr vaddr);

void *adb_try_get_write(addr_book *adb, addr_book_vaddr vaddr);

void adb_unlock(addr_book *adb, addr_book_vaddr vaddr);

void adb_free(addr_book *adb, addr_book_vaddr vaddr);

uint64_t adb_get_fill(addr_book *adb);

void adb_print(addr_book *adb);

#endif
