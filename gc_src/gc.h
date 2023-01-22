#ifndef GC_GC_H
#define GC_GC_H

#include <stdlib.h>
#include <time.h>
#include <stdint.h>

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
