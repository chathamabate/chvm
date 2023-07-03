#ifndef GC_CS_H
#define GC_CS_H

#include "mb.h"
#include "virt.h"
#include "ms.h"

#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#include <time.h>

// A gc_space will be one level of abstraction above mem_space.
// Interfacing with a gc_space will promise consistent structure
// of all memory pieces inside the mem_space being used.
//
// All pieces will be in the form of an "object" which holds data
// and references.
//
// This structure will allow for garbage collection!

typedef struct collected_space_struct collected_space;

typedef struct {
    // The number of references in the reference table.
    const uint64_t rt_len;

    // The number of bytes in the data array.
    const uint64_t da_size;
} obj_header;

typedef struct {
    obj_header *h;
    addr_book_vaddr *rt;
    uint8_t *da;
} obj_index;

static inline obj_index obj_h_to_index(obj_header *h) {
    obj_index obj_i;

    obj_i.h = h;
    obj_i.rt = (addr_book_vaddr *)(h + 1);
    obj_i.da = (uint8_t *)(obj_i.rt + h->rt_len);

    return obj_i;
}

// Here we provide information to set up the underlying memory space.
// NOTE: No root objects will be created at first.
collected_space *new_collected_space_seed(uint64_t chnl, uint64_t seed, 
        uint64_t adb_t_cap, uint64_t mb_m_bytes);

static inline collected_space *new_collected_space(uint64_t chnl,
        uint64_t adb_t_cap, uint64_t mb_m_bytes) {
    return new_collected_space_seed(chnl, time(NULL), adb_t_cap, 
            mb_m_bytes);
}

// Make sure no GC Thread is running while this is called.
// GC Background thread must be stopped before calling this.
void delete_collected_space(collected_space *cs);

// NOTE: After lots of thought, I have decided that the user
// will have access to object locks as originally intended...
// 
// That is, the user will be able to call gc_get_read/write.
// This will allow the user to stop the garbage collector
// indefinitely if they're not careful.
//
// However, other options are quite overkill and probably
// extremely slow.
//
// We will have to make sure compilers for the VM know this.

typedef struct {
    addr_book_vaddr vaddr;
    obj_index i;
} malloc_obj_res;
 
// This create an object in the underlying mem space and returns its
// information (Depending on whether hold is provided.
malloc_obj_res cs_malloc_object_p(collected_space *cs, uint64_t rt_len, 
        uint64_t da_size, uint8_t hold);

static inline addr_book_vaddr cs_malloc_object(collected_space *cs,
        uint64_t rt_len, uint64_t da_size) {
    return cs_malloc_object_p(cs, rt_len, da_size, 0).vaddr;
}

static inline malloc_obj_res cs_malloc_object_and_hold(collected_space *cs,
        uint64_t rt_len, uint64_t da_size) {
    return cs_malloc_object_p(cs, rt_len, da_size, 1);
}

uint8_t cs_allocated(collected_space *cs, addr_book_vaddr vaddr);

typedef uint64_t cs_root_id;

typedef enum {
    CS_SUCCESS = 0,

    // This is used when an out of bounds root id is given.
    CS_ROOT_ID_OUT_OF_BOUNDS,

    // This is used when the root id given is not allocated yet.
    CS_ROOT_ID_NOT_ALLOCATED,
} cs_root_status_code;


// NOTE: see implementation file to see notes on how rooting works in
// the presence of GC.
//
// NOTE: This allows for the same object to made an object twice.
// not sure why you'd do this, but won't break anything.
//
// Turn a normal object into a root object.
// Returns root id of new root.
cs_root_id cs_root(collected_space *cs, addr_book_vaddr vaddr);

// Create a root object from scratch.
cs_root_id cs_malloc_root(collected_space *cs, uint64_t rt_len, uint64_t da_size);

// Turn a root object into a normal object.
cs_root_status_code cs_deroot(collected_space *cs, cs_root_id root_id);

typedef struct {
    cs_root_status_code status_code;
    addr_book_vaddr root_vaddr;
} cs_get_root_res;

cs_get_root_res cs_get_root_vaddr(collected_space *cs, cs_root_id root_id);

// These calls are forwarded directly to the memory space.
obj_header *cs_get_read(collected_space *cs, addr_book_vaddr vaddr);

static inline obj_index cs_get_read_ind(collected_space *cs, 
        addr_book_vaddr vaddr) {
    return obj_h_to_index(cs_get_read(cs, vaddr));
}

// NOTE: This has special behavoir in the presence of GC.
obj_header *cs_get_write(collected_space *cs, addr_book_vaddr vaddr);

static inline obj_index cs_get_write_ind(collected_space *cs, 
        addr_book_vaddr vaddr) {
    return obj_h_to_index(cs_get_write(cs, vaddr));
}

void cs_unlock(collected_space *cs, addr_book_vaddr vaddr);

uint64_t cs_count(collected_space *cs);
void cs_print(collected_space *cs);
void cs_print_ms(collected_space *cs);

// Run garbage collection algorithm.
// See implementation file for notes.
//
// Returns number of objects collected.
uint64_t cs_collect_garbage(collected_space *cs);

typedef struct {
    const struct timespec *delay;
    
    // 1 if memory shifting should be done also.
    // 0 otherwise.
    uint8_t shift;

    // If shifting is on, this will equal the total number of
    // frees which must occur before a full shift triggers.
    uint64_t shift_trigger;
} gc_worker_spec;

// This will run a gc cycle every delay period.
// If gc is already on, nothing happens.
//
// returns 0 if the gc worker was started.
// returns 1 otherwise.
uint8_t cs_start_gc(collected_space *cs, const gc_worker_spec *spec);

// If a GC Thread is running when this is called, it will be told to stop,
// this thread will then wait until the GC Thread actually stops.
// Finally, 0 will be returned.
//
// This will return 1 if no GC Thread is running, or if it has
// already been told to stop.
// NOTE: just because this call returns 1, doesn't mean the GC thread
// isn't running! It may still be in the process of stopping itself.
//
// Thus, I would recommend not using this call in parallel with itself.
// Maybe fix this later.
uint8_t cs_stop_gc(collected_space *cs);

// Run try full shift on the underlying memory space.
void cs_try_full_shift(collected_space *cs);

#endif
