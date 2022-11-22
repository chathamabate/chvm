#ifndef GC_MEM_H
#define GC_MEM_H

#include "../core_src/mem.h"

// Here we will use a reference counting approach
// to garbage collection.
//
// Probably not the fastest style, but should be
// good enough for now.

// An addr will either be 0 (NULL) or the value of the address
// of a valid memory block. (void *)
//
// A valid memory block will look as follows :
//
// Block :
//      uint64_t    ref_count
//      uint64_t    rt_len             (rt for reference table)
//      uint64_t    da_len             (da for data array)
//      addr        rt[rt_len - 1]           
//      uint8_t     da[da_len]
//
// A block is broken into two areas: the reference table and
// the data array. This is done so that the garbage collector will
// always be able to identify references held inside a block
// regardless of what the block represents.

// mb for memory block.
static inline uint64_t mb_ref_count(void *mb) {
    return ((uint64_t *)mb)[0];
}

static inline uint64_t mb_rt_len(void *mb) {
    return ((uint64_t *)mb)[1];
}

static inline uint64_t mb_da_len(void *mb) {
    return ((uint64_t *)mb)[2];
}

static inline void **mb_rt(void *mb) {
    return (void **)((uint64_t *)mb + 3);
}



#endif
