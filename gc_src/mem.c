#include "./mem.h"
#include "virt.h"
#include <stdlib.h>
#include "../core_src/mem.h"
#include "../core_src/thread.h"






// New Memory Block Concept and Notes:
//
// A Memory block will be one contiguous piecce of memory
// that the user will be able to malloc into.
//
// Memory blocks will be broken into pieces.
// A free piece is a section of the memory block which is yet to
// be allocated by the user. 
// An occupied piece will be a section of the memory block which
// has been allocated by the user.
//
// A Memory Block will add some support for "shifting".
// This is the act of pushing all occupied pieces to the start of
// the memory block. The result being all free pieces being combined
// into one large free piece at the end of the block. 
//
// Memory Blocks will be large. (Probs >= 100kb)
// This way, shifting only needs to be done on a per Memory Block
// basis. As of now, data will never be shifted from
// one block to another.
//
// If the user wants a piece of memory larger than the default
// block size, a custom size block will be created.
// 
// The shifting code will need to be aware of the address book.
// Each piece of occupied memory will store its corresponding
// virtual address. (Memory Blocks must be aware of the address book)
//
// Memory Blocks will not be aware of garbage collection strategies.
// Garbage collection should be able to use the Memory Block API
// to do all of its work.
//
// Pieces of Memory will have the following structure:
//
// size will always be divisible by two. 
// (size will include the Header and Footer)
// alloc will reside in the first bit. (Whether or not the piece ic occupied)
//
// size_t size | alloc; (Header)
//
// ... size - (2 * sizeof(size_t)) bytes ... (Body)
//
// size_t size | alloc; (Footer)
//
// If occupied/allocated, the body will take the folowing structure:
//
// The virtual address which corresponds to this block of memory.
// addr_book_vaddr vaddr;
// ... (originally requested number of bytes) ... 
// Padding (if needed)
//
// If unoccupied/free, the body will have this structure:
//
// TODO.........
//
//
// Unsolved Questions:
//
// Will Address books be responsible for setting up
// specific pieces of memory???
// How will pieces of memory be organized???
//
// 
// How will the free list be organized??
// How will shifting be implemented??
// When we shift, must we freeze the entire block???
// Is there a way to shift individual pieces of memory one by one??








// Might need to rewrite all my code below...
// As it no longer really works...
//
// What is the best way to shift around memory?
// How should our memory structures be organized 
// to optimize memory shifting.
// 
// I will use close to constant size memory pages.
// Each memory page will have a free list of its own.
// Preferably, there will be some free list organization
// strategy. For example, sorted by free block size.
//
// There will be a memory space structure for working with
// memory pages. The memory space will have the ability 
// to manage memory shifting. 
//
// The goal of shifting is to defragment the memory space.
//
// How will the memory space shift memory effectively and 
// concurrently. i.e. We need a shifting method that actually
// produces a defragmented memory space, and does not stop
// the user from using malloc and free as normal.
//
// How will memory pages be sorted?
//
// Will creating an entirely new chain of memory pages make
// memory shifting easier to implement??? 
//
// We should only shift around memory it is has the potential
// of creating large free blocks in the future.

typedef struct  {
    // Number of bytes in the data section.
    const uint64_t cap;

    // Pointer to the first block in the free list.
    //
    // NOTE: Free list will always be sorted in order
    // of descending size. (In a singly linked list)
    //
    // NOTE: Whenever writing or reading to any item
    // in the free list. (Including the free_list ptr)
    // The free_lck should be acquired.
    pthread_rwlock_t free_lck;
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

static inline uint64_t blk_alloc_size(void *blk) {
    // Neglect the header here.
    return blk_size(blk) - sizeof(uint64_t);
}

static inline void *blk_body(void *blk) {
    return (uint64_t *)blk + 1;
}

static inline void *blk_next_free(void *blk) {
    return *(void **)blk_body(blk);
}

static inline void *blk_next(void *blk) {
    return (uint8_t *)blk + blk_size(blk);
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
    uint64_t padded_size = blk_pad_size(min_bytes);
    mem_page *mp = safe_malloc(chnl, sizeof(mem_page_header) + padded_size);
    mem_page_header *mp_h = (mem_page_header *)mp;

    *(uint64_t *)&(mp_h->cap) = padded_size;
    
    // Make the entire body a single free block.
    void *body = (void *)mp_h + 1;
    *(uint64_t *)body = padded_size;
    *(void **)((uint64_t *)body + 1) = NULL;

    mp_h->free_list = body;
    safe_rwlock_init(&(mp_h->free_lck), NULL);

    return mp;
}

void delete_mem_page(mem_page *mp) {
    mem_page_header *mp_h = (mem_page_header *)mp;
    safe_rwlock_destroy(&(mp_h->free_lck));
    safe_free(mp);
}

uint64_t mp_get_space(mem_page *mp) {
    mem_page_header *mp_h = (mem_page_header *)mp;

    uint64_t num_bytes;

    safe_rdlock(&(mp_h->free_lck));

    // Check if the free list is empty.
    if (!(mp_h->free_list)) {
        safe_rwlock_unlock(&(mp_h->free_lck));
        return 0;
    }

    num_bytes = blk_size(mp_h->free_list);

    safe_rwlock_unlock(&(mp_h->free_lck));

    // Remember not to count the header.
    return num_bytes - sizeof(uint64_t);
}

void *mp_malloc(mem_page *mp, uint64_t min_bytes) {
    mem_page_header *mp_h = (mem_page_header *)mp;

    safe_rdlock(&(mp_h->free_lck));

    void *prev = NULL;
    void *iter = mp_h->free_list;

    // go until we find a block with size less than or equal to
    // min_bytes.
    while (iter && blk_alloc_size(iter) > min_bytes) {
        prev = iter;
        iter = blk_next_free(iter);
    }

    // If we have found a block of size equal to min bytes,
    // iterate once.
    if (iter && blk_alloc_size(iter) == min_bytes) {
        prev = iter;
        iter = blk_next_free(iter);
    }

    // prev will be the malloced block.
    // If no blocks of size greater than or equal to min_bytes
    // were iterated over, prev will be NULL at this point.
    if (!prev) {
        safe_rwlock_unlock(&(mp_h->free_lck));
        return NULL;
    }

    // OOP we got a mistake :,(
    
    safe_rwlock_unlock(&(mp_h->free_lck));

    return NULL;
}

void mp_free(mem_page *mp, void *ptr) {
    
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
