#ifndef GC_MB_H
#define GC_MB_H

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

// This returns the largest call to malloc possible
// for the given memory block at the given time.
uint64_t mb_free_space(mem_block *mb);

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
// This call will return MB_BUSY if there are
// no unlocked pieces to shift. 
mb_shift_res mb_try_shift(mem_block *mb);

// Shift while there are unlocked blocks to shift.
static inline void mb_try_full_shift(mem_block *mb) {
    while (mb_try_shift(mb) == MB_SHIFT_SUCCESS);
}

// This command will safely print the structure of the 
// memory block in an easy to read way.
// Mainly for easy debugging.
void mb_print(mem_block *mb);

#endif
