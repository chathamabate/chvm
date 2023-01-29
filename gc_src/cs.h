#ifndef GC_CS_H
#define GC_CS_H

#include "virt.h"
#include "ms.h"

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

typedef struct collected_space_struct collected_space;

typedef struct {
    // The number of references in the reference table.
    const uint64_t rt_len;

    // The number of bytes in the data array.
    const uint64_t da_size;
} obj_header;

// Here we provide information to set up the underlying memory space.
// NOTE: No root objects will be created at first.
collected_space *new_collected_space_seed(uint64_t chnl, uint64_t seed, 
        uint64_t adb_t_cap, uint64_t mb_m_bytes);

static inline collected_space *new_collected_space(uint64_t chnl,
        uint64_t adb_t_cap, uint64_t mb_m_bytes) {
    return new_collected_space_seed(chnl, time(NULL), adb_t_cap, 
            mb_m_bytes);
}

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

// The gc_space will hold an array list of "root objects".
// A "root object" is always live (that is it will never
// be garbage collected)
// The reference out from roots will determine which objects
// are not GC'd.
//
// Users refer to root objects via an index.
// Users refer to references in a root object via an offset.

// This create an object in the underlying mem space and returns its
// information (Depending on whether hold is provided.
malloc_res cs_malloc_object_p(collected_space *cs, uint64_t rt_len, 
        uint64_t da_size, uint8_t hold);

static inline addr_book_vaddr cs_malloc_object(collected_space *cs,
        uint64_t rt_len, uint64_t da_size) {
    return cs_malloc_object_p(cs, rt_len, da_size, 0).vaddr;
}

static inline malloc_res cs_malloc_object_and_hold(collected_space *cs,
        uint64_t rt_len, uint64_t da_size) {
    return cs_malloc_object_p(cs, rt_len, da_size, 1);
}

// NOTE: somehow restrictions will need to be put in place
// by the assembler to make sure the user doesn't f everything up.
// Or maybe they should be allowed to?
//
// NOTE: root and deroot assume the user doesn't already have the lock on the given 
// object! What about a fixed size array?? The user decides???

typedef uint64_t cs_root_id;

typedef enum {
    CS_SUCCESS = 0,

    // This is used when you try to promote an object which
    // is already a root. 
    CS_ALREADY_ROOT,

    // This is used when you try to deroot and object which
    // is not a root.
    CS_NOT_A_ROOT, 

    // This is used when an out of bounds root id is given.
    CS_ROOT_ID_OUT_OF_BOUNDS,

    // This is used when the root id given is not allocated yet.
    CS_ROOT_ID_NOT_ALLOCATED,
} cs_root_status_code;


typedef struct {
    cs_root_status_code status_code;
    cs_root_id root_id;
} cs_root_res;

// Turn a normal object into a root object.
// NOTE: if a given object is already a root UINT64_MAX is returned.
cs_root_res cs_root(collected_space *cs, addr_book_vaddr vaddr);

// Create a root object from scratch.
cs_root_id cs_malloc_root(collected_space *cs, uint64_t rt_len, uint64_t da_size);

// Turn a root object into a normal object.
// If something is derooted 0 is returned.
// (Otherwise) if root_id is invalid return 1.
cs_root_status_code cs_deroot(collected_space *cs, cs_root_id root_id);

typedef struct {
    cs_root_status_code status_code;
    addr_book_vaddr root_vaddr;
} cs_get_root_res;

// Will return NULL if a bad root_id is given.
cs_get_root_res cs_get_root_vaddr(collected_space *cs, cs_root_id root_id);

// These calls are forwarded directly to the memory space.
obj_header *cs_get_read(collected_space *cs, addr_book_vaddr vaddr);
obj_header *cs_get_write(collected_space *cs, addr_book_vaddr vaddr);
void cs_unlock(collected_space *cs, addr_book_vaddr vaddr);

void cs_print(collected_space *cs);

// Run garbage collection algorithm.
void cs_collect_garbage(collected_space *cs);

// Run try full shift on the underlying memory space.
void cs_try_full_shift(collected_space *cs);

#endif
