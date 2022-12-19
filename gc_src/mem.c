#include "./mem.h"
#include "virt.h"
#include <stdlib.h>
#include "../core_src/mem.h"

typedef struct  {
    // Number of bytes in the data section.
    uint64_t cap;

} mem_page_header;

mem_page *new_mem_page(uint8_t chnl, uint64_t cap) {
    return NULL;
}

void delete_mem_page(mem_page *mp) {

}

uint64_t mp_get_cap(mem_page *mp) {
    return 0;
}

// The size of the largest free block in the memory
// page.
uint64_t mp_get_space(mem_page *mp) {
    return 0;
}

// Will return NULL if there isn't enough space at the time
// of request.
void *mp_malloc(uint64_t bytes) {
    return NULL;
}

void mp_free(void *ptr) {

}












// NOTE: EVERYTHING BELOW IS FOR LATER!!!!!

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
