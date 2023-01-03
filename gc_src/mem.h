#ifndef GC_MEM_H
#define GC_MEM_H

#include <stdint.h>
#include "./virt.h"

typedef struct {} mem_block;

mem_block *new_mem_block(uint8_t chnl, addr_book *adb, uint64_t min_bytes);

// NOTE: this will also free all vaddrs in this block's corresponding 
// address book.
// 
// NOTE: This does not delete address book referenced by the memory
// block.
void delete_mem_block(mem_block *mb);

// Will return NULL_VADDR on failure.
// (Maybe improve the detail of this return type when 
// implementing memory space)
addr_book_vaddr mb_malloc(mem_block *mb, uint64_t min_bytes);

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
// If blk = 0, this call will return MB_BUSY if there are
// no unlocked pieces to shift. 
//
// If blk = 1, and there are only locked pieces left to shift,
// this call will block until it can acquire the lock of 
// one of the shiftable pieces.
mb_shift_res mb_shift_p(mem_block *mb, uint8_t blk);

static inline mb_shift_res mb_try_shift(mem_block *mb) {
    return mb_shift_p(mb, 0);
}

static inline mb_shift_res mb_shift(mem_block *mb) {
    return mb_shift_p(mb, 1);
}

static inline void mb_full_shift(mem_block *mb) {
    while (mb_shift_p(mb, 0) != MB_NOT_NEEDED);
}

// After this call, all free pieces will be pushed together
// to form one single free piece at one end of the block.
// static inline void mb_full_shift(mem_block *mb) {
//    while (mb_shift(mb) != MB_NOT_NEEDED);
//}

// This command will safely print the structure of the 
// memory block in an easy to read way.
// Mainly for easy debugging.
void mb_print(mem_block *mb);

#endif
