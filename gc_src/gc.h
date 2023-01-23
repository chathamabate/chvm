#ifndef GC_GC_H
#define GC_GC_H

#include <stdlib.h>
#include <time.h>
#include <stdint.h>

// A gc_space will be one level of abstraction above mem_space.
// Interfacing with a gc_space will promise consistent structure
// of all memory pieces inside the mem_space being used.
//
// All pieces will be in the form of an "object" which holds data
// and references.
//
// This structure will allow for garbage collection!

typedef struct gc_space_struct gc_space;

gc_space *new_gc_space_seed(uint64_t chnl, uint64_t seed, 
        uint64_t adb_t_cap, uint64_t mb_m_bytes,
        uint64_t root_rt_len);

static inline gc_space *new_gc_space(uint64_t chnl,
        uint64_t adb_t_cap, uint64_t mb_m_bytes,
        uint64_t root_rt_len) {
    return new_gc_space_seed(chnl, time(NULL), adb_t_cap, 
            mb_m_bytes, root_rt_len);
}

void delete_gc_space(gc_space *gc);



#endif
