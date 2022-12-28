#include "./mem.h"
#include "virt.h"
#include <stdlib.h>

#include "../core_src/io.h"
#include "../core_src/mem.h"
#include "../core_src/thread.h"

#include "../core_src/io.h"
#include <inttypes.h>

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
// Free list pointers. A free memory piece will be in two free lists.
// One will be sorted by address of the free block.
// The other sorted by size.
//
// void *addr_prev;
// void *addr_next;
//
// void *size_prev;
// void *size_next;
//
// Unsolved Questions:
// 
// What assumptions are needed for thread safety??
// How will the free list be sorted???
// How many locks will the block have??
// Maybe just one free list lock???
//
// Will Address books be responsible for setting up
// specific pieces of memory???
// How will pieces of memory be organized???
//
// How will the free list be organized??
// How will shifting be implemented??
// When we shift, must we freeze the entire block???
// Is there a way to shift individual pieces of memory one by one??

// As of now, all operations on free blocks will be atomic.

typedef struct {} mem_piece;

static const uint64_t MP_ALLOC_MASK = 0x1;
static const uint64_t MP_SIZE_MASK = ~MP_ALLOC_MASK;

static const uint64_t MP_PADDING = 2 * sizeof(uint64_t);

static inline void mp_init(mem_piece *mp, uint64_t size, uint8_t alloc) {
    uint64_t tag = size | alloc;

    *(uint64_t *)mp = tag;
    ((uint64_t *)((uint8_t *)mp + size))[-1] = tag;
}

static inline uint64_t mp_size(mem_piece *mp) {
    return *(uint64_t *)mp & MP_SIZE_MASK;
}

static inline uint8_t mp_alloc(mem_piece *mp) {
    return (uint8_t)(*(uint64_t *)mp & MP_ALLOC_MASK);
}

static inline void *mp_body(mem_piece *mp) {
    return (void *)((uint64_t *)mp + 1);
}

static inline mem_piece *mp_b_to_mp(void *mp_b) {
    return (mem_piece *)((uint64_t *)mp_b - 1);
}

static inline mem_piece *mp_next(mem_piece *mp) {
    return (mem_piece *)((uint8_t *)mp + mp_size(mp));
}

static inline uint64_t mp_prev_size(mem_piece *mp) {
    return ((uint64_t *)mp)[-1] & MP_SIZE_MASK; 
}

static inline mem_piece *mp_prev(mem_piece *mp) {
    return (mem_piece *)((uint8_t *)mp - mp_prev_size(mp));
}

typedef struct mem_free_piece_header_struct {
    struct mem_free_piece_header_struct *size_free_prev;
    struct mem_free_piece_header_struct *size_free_next;
} mem_free_piece_header;

static const uint64_t MFP_PADDING = MP_PADDING + sizeof(mem_free_piece_header);

// This is a little confusing.
// Returns the amount of user accessible memory which can be allocated in
// the given free block.
static inline uint64_t mfp_h_free_space(mem_free_piece_header *mfp_h) {
    return mp_size(mp_b_to_mp(mfp_h)) - MFP_PADDING;
}

typedef addr_book_vaddr mem_alloc_piece_header;

// Convert the pointer to the body of an allocated block
// into a pointer to a memory piece.
static inline mem_piece *map_b_to_mp(void *map_b) {
    return mp_b_to_mp((mem_alloc_piece_header *)map_b - 1);
}

static inline void *mp_to_map_b(mem_piece *mp) {
    return (mem_alloc_piece_header *)mp_body(mp) + 1;
}

static const uint64_t MAP_PADDING = MP_PADDING + sizeof(mem_alloc_piece_header);

// NOTE: The value of this constant MUST be even.
// Also note, that this value depends on what is stored in 
// the alloc_piece_header. 
//
// NOTE: right now we have the plus 1 since MFP_PADDING = MAP_PADDING.
// A free block should be mallocable for at least 1 byte.
//
// This should always store whichever padding value is larger.
static const uint64_t MP_MIN_SIZE = MFP_PADDING + 1;

// Round the given number of bytes to be divisible by two.
static inline uint64_t round_num_bytes(uint64_t num_bytes) {
    return num_bytes % 2 ? num_bytes + 1 : num_bytes;
}

// Pad number of bytes by the allocated piece padding, and round.
static inline uint64_t pad_num_bytes(uint64_t num_bytes) {
    uint64_t map_size = round_num_bytes(num_bytes) + MAP_PADDING; 

    // Must have at least minimum size!
    // (Accounting for when a block is freed)
    return map_size < MP_MIN_SIZE ? MP_MIN_SIZE : map_size;
}

typedef struct {
    // Number of bytes following this header.
    const uint64_t cap;

    // Address book for all allocated piece virtual
    // addresses.
    addr_book * const adb;

    // This lock should be used whenever the structure of
    // the memory block is being changed or read. 
    //
    // For example, a piece is being allocated, or
    // the headers of a piece are being read, etc...
    pthread_rwlock_t mem_lck;

    mem_free_piece_header *size_free_list; 
} mem_block_header;

// How do we confirm an allocated block is not being modified 
// as we retrieve its vaddr?????

mem_block *new_mem_block(uint8_t chnl, addr_book *adb, uint64_t min_bytes) {
    uint64_t padded_cap = pad_num_bytes(min_bytes);
    mem_block *mb = safe_malloc(chnl, sizeof(mem_block_header) + padded_cap);

    mem_block_header *mb_h = (mem_block_header *)mb;

    *(uint64_t *)&(mb_h->cap) = padded_cap;
    *(addr_book **)&(mb_h->adb) = adb;

    safe_rwlock_init(&(mb_h->mem_lck), NULL);

    // Get the first memory piece.
    mem_piece *mp = (mem_piece *)(mb_h + 1);
    mp_init(mp, padded_cap, 0);

    mem_free_piece_header *mfp_h = (mem_free_piece_header *)mp_body(mp);

    mfp_h->size_free_next = NULL;
    mfp_h->size_free_prev = NULL;

    mb_h->size_free_list = mfp_h;

    return mb;
}

void delete_mem_block(mem_block *mb) {
    mem_block_header *mb_h = (mem_block_header *)mb;

    mem_piece *start  = (mem_piece *)(mb_h + 1);
    mem_piece *end = (mem_piece *)((uint8_t *)start + mb_h->cap);

    // Again, we lock here, but really, this call should never be
    // called in parallel with any other call to the same mem
    // block.
    safe_wrlock(&(mb_h->mem_lck));

    mem_piece *iter = start;

    // Here we free all virtual addresses used by the
    // memory block.
    while (iter < end) {
        // Only free vaddrs from allocated blocks.
        if (mp_alloc(iter)) {
            mem_alloc_piece_header *map_h = 
                (mem_alloc_piece_header *)mp_body(iter);

            adb_free(mb_h->adb, *map_h);
        }

        iter = mp_next(iter);
    }

    safe_rwlock_unlock(&(mb_h->mem_lck));

    safe_free(mb);
}

// Remove piece from size free list only.
static void mb_remove_from_size_unsafe(mem_block *mb, mem_free_piece_header *mfp_h) {
    mem_block_header *mb_h = (mem_block_header *)mb;

    if (mfp_h->size_free_next) {
        mfp_h->size_free_next->size_free_prev = mfp_h->size_free_prev;
    }

    if (mfp_h->size_free_prev) {
        mfp_h->size_free_prev->size_free_next = mfp_h->size_free_next;
    } else {
        mb_h->size_free_list = mfp_h->size_free_next;
    }
}

// Add a piece which does not currently reside in the size free list into the
// size free list.
static void mb_add_to_size_unsafe(mem_block *mb, mem_piece *mp) {
    mem_block_header *mb_h = (mem_block_header *)mb;

    uint64_t size = mp_size(mp); 
    mem_free_piece_header *mfp_h = (mem_free_piece_header *)mp_body(mp);

    mem_free_piece_header *iter = mb_h->size_free_list;
    mem_free_piece_header *next_iter = NULL;

    if (!iter) {
        mb_h->size_free_list = mfp_h;

        mfp_h->size_free_prev = NULL;
        mfp_h->size_free_next = NULL;

        return;
    }

    // Go until our iterator has a size <= mp.
    while (mp_size(mp_b_to_mp(iter)) > size) {
        next_iter = iter->size_free_next;

        // Here we have made it to the end of 
        // our list.
        if (!next_iter) {
            iter->size_free_next = mfp_h; 

            mfp_h->size_free_prev = iter;
            mfp_h->size_free_next = NULL;

            return;
        }

        iter = next_iter;
    }

    if (iter->size_free_prev) {
        iter->size_free_prev->size_free_next = mfp_h;
    } else {
        mb_h->size_free_list = mfp_h;
    }

    // Insert mp just before iter.
    mfp_h->size_free_next = iter;
    mfp_h->size_free_prev = iter->size_free_prev;

    iter->size_free_prev = mfp_h;
}

// mp will be a newly freed piece which is not part of any free lists yet.
static void mb_free_unsafe(mem_block *mb, mem_piece *mp) {
    mem_block_header *mb_h = (mem_block_header *)mb;

    mem_piece *start = (mem_piece *)(mb_h + 1);
    mem_piece *end = (mem_piece *)((uint8_t *)(mb_h + 1) + mb_h->cap);

    // No matter the situation, the new free block must be newly added
    // into the size free list. 
    
    mem_piece *prev = NULL;
    uint8_t prev_free = 0;

    if (mp > start) {
        // Only calculate prev if it is in bounds.
        prev = mp_prev(mp); 
        prev_free = !mp_alloc(prev);
    }

    mem_piece *next = mp_next(mp);
    uint8_t next_free = 0;

    if (next < end) {
        next_free = !mp_alloc(next); 
    } else {
        // void the next pointer if it is out of bounds.
        next = NULL;     
    }

    // Now, get needed free piece headers and remove pieces
    // from size free list.
    mem_piece *new_free = mp;
    uint64_t new_size = mp_size(mp);
    
    mem_free_piece_header *prev_mfp_h;
    if (prev_free) {
        prev_mfp_h = (mem_free_piece_header *)mp_body(prev);
        mb_remove_from_size_unsafe(mb, prev_mfp_h);

        new_free = prev;
        new_size += mp_size(prev);
    }

    mem_free_piece_header *next_mfp_h;
    if (next_free) {
        next_mfp_h = (mem_free_piece_header *)mp_body(next);
        mb_remove_from_size_unsafe(mb, next_mfp_h);

        new_size += mp_size(next);
    }

    mp_init(new_free, new_size, 0);
    mb_add_to_size_unsafe(mb, new_free);
}

void mb_free(mem_block *mb, addr_book_vaddr vaddr) {
    mem_block_header *mb_h = (mem_block_header *)mb;

    safe_wrlock(&(mb_h->mem_lck));

    // After acquring mem_lck, we know we cannot be mid shift.
    // Thus, physical addresses will be constant.

    // We get our physical address in an arbitrary manner.
    //
    // NOTE: We assume that no one is reading or writing to vaddr
    // at the time of this call. The only exception being during a 
    // shift... However, this is the purpose of the mem_lck.
    void *paddr = adb_get_read(mb_h->adb, vaddr);
    adb_unlock(mb_h->adb, vaddr);

    // Discard vaddr entirely.
    adb_free(mb_h->adb, vaddr);

    // Get corresponding mem_piece pointer.
    mem_piece *mp = map_b_to_mp(paddr);
    mb_free_unsafe(mb, mp);
    
    safe_rwlock_unlock(&(mb_h->mem_lck)); 
}

addr_book_vaddr mb_malloc(mem_block *mb, uint64_t min_bytes) {
    // Never allocate an empty piece!
    if (min_bytes == 0) {
        return NULL_VADDR;
    } 

    mem_block_header *mb_h = (mem_block_header *)mb;

    // Must account for a lot for headers and vaddr.
    uint64_t min_size = pad_num_bytes(min_bytes);

    safe_wrlock(&(mb_h->mem_lck)); 

    mem_free_piece_header *big_free_h = mb_h->size_free_list;

    if (!big_free_h) {
        safe_rwlock_unlock(&(mb_h->mem_lck));

        return NULL_VADDR;
    }

    mem_piece *big_free = mp_b_to_mp(big_free_h);
    uint64_t big_free_size = mp_size(big_free);

    if (big_free_size < min_size) {
        safe_rwlock_unlock(&(mb_h->mem_lck));

        return NULL_VADDR;
    }

    // As we must have enough space, we pop our big free block
    // off the front of the size free list.
    mb_h->size_free_list = big_free_h->size_free_next;

    if (big_free_h->size_free_next) {
        big_free_h->size_free_next->size_free_prev = NULL;
    }

    uint64_t cut_size = big_free_size - min_size;

    // Here we check to see if we should divide our big free block.
    // This is only done when there are enough remaining bytes 
    // to malloc at least once. 
    if (cut_size < MP_MIN_SIZE) {
        // Here there will be no division.
        // Simply remove big free from corresponding free
        // lists.             

        mp_init(big_free, big_free_size, 1);
        safe_rwlock_unlock(&(mb_h->mem_lck));

        return adb_install(mb_h->adb, mp_to_map_b(big_free));
    }

    // Here we cut!
    mem_piece *new_free = (mem_piece *)((uint8_t *)big_free + min_size);
    mp_init(new_free, cut_size, 0);

    // Add new cut to size free list.
    mb_add_to_size_unsafe(mb, new_free);

    // Finally, init our new allocated block.
    mp_init(big_free, min_size, 1);
    safe_rwlock_unlock(&(mb_h->mem_lck));

    return adb_install(mb_h->adb, mp_to_map_b(big_free));
}

void mb_shift(mem_block *mb) {
    // TODO... write this at some point.
}

void mb_print(mem_block *mb) {
    mem_block_header *mb_h = (mem_block_header *)mb;

    safe_rdlock(&(mb_h->mem_lck));

    mem_piece *start  = (mem_piece *)(mb_h + 1);
    mem_piece *end = (mem_piece *)((uint8_t *)start + mb_h->cap);

    uint64_t piece_num = 0;
    mem_piece *iter = start;

    while (iter < end) {
        safe_printf("%" PRIu64 " : %p : Size %" PRIu64 " : ", 
                piece_num, iter, mp_size(iter)); 

        if (mp_alloc(iter)) {
            safe_printf("Allocated\n");
        } else {
            safe_printf("Free\n");

            mem_free_piece_header *mfp_h = 
                (mem_free_piece_header *)mp_body(iter);
            
            if (mfp_h->size_free_prev) {
                safe_printf("  Prev : %p\n", mp_b_to_mp(mfp_h->size_free_prev));
            }

            if (mfp_h->size_free_next) {
                safe_printf("  Next : %p\n", mp_b_to_mp(mfp_h->size_free_next));
            }
        }
        
        piece_num++;
        iter = mp_next(iter);
    }

    safe_rwlock_unlock(&(mb_h->mem_lck));
}

