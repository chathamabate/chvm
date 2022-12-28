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

// This will shift pieces to the start of the memory block.
//
// NOTE: This will write lock on allocated pieces inside
// the memory block. Make sure you are not locking
// on any piece before calling this function. 
// (I mean in the same thread btw)
//
// Returns 1 if a shift occurred, 0 otherwise.
// We only don't shift if there are no blocks
// to shift.
uint8_t mb_shift(mem_block *mb);

// This command will safely print the structure of the 
// memory block in an easy to read way.
// Mainly for easy debugging.
void mb_print(mem_block *mb);

#endif
