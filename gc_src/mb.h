#ifndef GC_MB_H
#define GC_MB_H

#include <stdint.h>
#include "./virt.h"

typedef struct {} mem_block;

mem_block *new_mem_block(uint8_t chnl, addr_book *adb, uint64_t min_bytes);

mem_block *new_mem_block_arr(uint8_t chnl, addr_book *adb, 
        uint64_t cell_size, uint64_t min_cells);

// NOTE: this will also free all vaddrs in this block's corresponding 
// address book.
// 
// NOTE: This does not delete address book referenced by the memory
// block.
void delete_mem_block(mem_block *mb);

// This returns the largest call to malloc possible
// for the given memory block at the given time.
uint64_t mb_free_space(mem_block *mb);

typedef struct {
    void *paddr;
    addr_book_vaddr vaddr;
} malloc_res;

// Will return NULL_VADDR on failure.
// (Maybe improve the detail of this return type when 
// implementing memory space)
//
// NOTE: I have realized an issue, when we malloc, the returned memory piece
// has not data at all initialized (Except for its virtaul address)
// If we decided to call malloc, then lock to initlalize the data,
// there is no promise that another thread doesn't access the piece 
// before the lock is acquired.
// This is specifically a problem with the garbage collector. When we iterate
// over a memory block's pieces, we want to be certain we aren't accessing any
// unitilized pieces!
//
// So, if hold is specified the piece will be malloced and its lock will be held
// even after this function returns. Addtionally, the paddr will be given in
// the corresponding result.
//
malloc_res mb_malloc_p(mem_block *mb, uint64_t min_bytes, uint8_t hold);

static inline addr_book_vaddr mb_malloc(mem_block *mb, uint64_t min_bytes) {
    return mb_malloc_p(mb, min_bytes, 0).vaddr;
}

static inline malloc_res mb_malloc_and_hold(mem_block *mb, 
        uint64_t min_bytes) {
    return mb_malloc_p(mb, min_bytes, 1);
}

void mb_free(mem_block *mb, addr_book_vaddr vaddr);

typedef enum {
    // This is returned when a shift is executed 
    // successfully on a single piece.
    MB_SHIFT_SUCCESS = 0,

    // This is returned when a shift was possible
    // however, the piece which NEEDED to be shifted
    // was locked on.
    // // NOTE: This should be quite rare. When the memory
    // block tries to shift a busy piece, it will simply 
    // move to another piece. This will only be returned
    // when the only pieces left to shift are all locked on.
    MB_BUSY,

    // This is returned when the memory block is
    // entirely shfited. That is there exists a
    // single free piece left, and it is pushed all
    // the way to one side of the block.
    MB_NOT_NEEDED,
} mb_shift_res;

// This call may shift a single allocated piece towards
// the beginning of the block.
//
// This call will return MB_BUSY if there are
// no unlocked pieces to shift. 
mb_shift_res mb_try_shift(mem_block *mb);

// Shift while there are unlocked blocks to shift.
static inline void mb_try_full_shift(mem_block *mb) {
    while (mb_try_shift(mb) == MB_SHIFT_SUCCESS);
}

typedef void (*mp_consumer)(addr_book_vaddr v, void *paddr, void *ctx);

// Iterate over every allocated piece in this memory block.
// If wr = 1, the write lock will be acquired on each piece.
// If wr = 0, the read lock will be.
void mb_foreach(mem_block *mb, mp_consumer c, void *ctx, uint8_t wr);

static inline void mb_foreach_read(mem_block *mb, mp_consumer c, void *ctx) {
    mb_foreach(mb, c, ctx, 0);
}

static inline void mb_foreach_write(mem_block *mb, mp_consumer c, void *ctx) {
    mb_foreach(mb, c, ctx, 1);
}

typedef uint8_t (*mp_predicate)(addr_book_vaddr v, void *paddr, void *ctx);

// This will never mutate. only ever aquire the read lock
// on memory pieces.
void mb_filter(mem_block *mb, mp_predicate pred, void *ctx);

// Number of allocated pieces in the memory block.
uint64_t mb_count(mem_block *mb);

// This command will safely print the structure of the 
// memory block in an easy to read way.
// Mainly for easy debugging.
void mb_print(mem_block *mb);

#endif
