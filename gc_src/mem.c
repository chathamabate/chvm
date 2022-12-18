#include "./mem.h"
#include "virt.h"

typedef struct {
    // Might at a total size field as well..
    

    uint8_t gc_flag; // Field to be used by GC algorithm.
    uint64_t rt_len; // Reference table len.
    uint64_t dt_len; // Data table len.
} data_block_header;

// After the header comes the reference table
// of exactly rt_len addr_book_vaddrs.
// After the reference table comes the data table
// of exactly dt_len bytes.

uint64_t db_get_refs_len(data_block *db) {
    return ((data_block_header *)db)->rt_len;
}

addr_book_vaddr *db_get_refs(data_block *db) {
    return (addr_book_vaddr *)((data_block_header *)db + 1);
}

uint64_t db_get_data_len(data_block *db) {
    return ((data_block_header *)db)->dt_len;
}

uint8_t *db_get_data(data_block *db) {
    data_block_header *db_h = (data_block_header *)db;
    addr_book_vaddr *rt = (addr_book_vaddr *)(db_h + 1);
    return (uint8_t *)(rt + db_h->rt_len);
}
