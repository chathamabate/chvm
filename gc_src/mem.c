#include "./mem.h"
#include "virt.h"
#include <stdlib.h>
#include "../core_src/mem.h"

typedef struct  {
    // Number of bytes in the data section.
    uint64_t cap;

    // Reading and writing to the free list should be thread 
    // safe???

    // Pointer to the first block in the free list.
    //
    // NOTE: Free list will always be sorted in order
    // of descending size. (In a singly linked list)
    void *free_list; 
} mem_page_header;

// After the mem_page_header comes an array of blocks
// all with the following structure :
//
// uint64_t header;
//
// header & 0x1 = if the block is allocated.
// header & ~(0x1) = number of bytes the block inhabits
// (always divisible by 2)
//
// If a block is not allocated it will next hold a single
// free list pointer. The an array of empty bytes depending
// on the size of the free block.
//
// void *next_free;
//
// If the block is allocated, it will hold just an array 
// of bytes. (A user cannot allocate less than sizeof(void *)
// bytes)

static const uint64_t BLK_ALLOC_MASK = 0x1;
static const uint64_t BLK_SIZE_MASK = ~BLK_ALLOC_MASK;

// Smallest block size (Because of the needed free pointer.
static const uint64_t BLK_MIN_SIZE = sizeof(uint64_t) + sizeof(void *);

static inline uint8_t blk_allocated(void *blk) {
    return *(uint64_t *)blk & BLK_ALLOC_MASK;
}

static inline uint64_t blk_size(void *blk) {
    return *(uint64_t *)blk & BLK_SIZE_MASK;
}

static inline void *blk_body(void *blk) {
    return (uint64_t *)blk + 1;
}

static inline void *blk_next_free(void *blk) {
    return *(void **)blk_body(blk);
}

static inline uint64_t blk_pad_size(uint64_t min_bytes) {
    // Account for header and even number.
    uint64_t padded_size = 
        sizeof(uint64_t) + min_bytes + (min_bytes & 0x1);

    if (padded_size < BLK_MIN_SIZE) {
        return BLK_MIN_SIZE;
    }

    return padded_size;
}

mem_page *new_mem_page(uint8_t chnl, uint64_t min_bytes) {
    return NULL;
}

void delete_mem_page(mem_page *mp) {

}

uint64_t mp_get_space(mem_page *mp) {
    return 0;
}

void *mp_malloc(uint64_t min_bytes) {
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
