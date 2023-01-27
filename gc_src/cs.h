#ifndef GC_CS_H
#define GC_CS_H

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

// NOTE: the user facing operations below will mimic memory orientated
// operations for the final VM.
//
// NOTE: A Collected space is thread safe in a way.
// That is calls in parrallel to any of the below endpoints
// will not result in fatal errors.
// The collected space will survive and its data structures
// will be intact.
// However, the order of the operations will not be deterministic.
// If one is doing writes and reads from the same object in parallel,
// god only knows which operations will happen when.
//
// For this reason, I think I will need to add some sort of mutex,
// creation operation below. Why not just force fast operations??
// That is the original design... the user can check out physcial
// addresses and do as they please with them... I just think this
// is too dangerous... 
//
// A user could then stop the garbage collector indefinitely.
// They could also totally destroy an object structure by
// accident. I prefer the below design despite its slow down.
//
// Maybe I should be brainstorming ways of having the below calls
// emit error handling information to be used by the final VM.
//
// ^ GOOD IDEA HERE!

// The gc_space will hold an array list of "root objects".
// A "root object" will just hold an table of references.
// A "root object" is always live (that is it will never
// be garbage collected)
//
// Users refer to root objects via an index.
// Users refer to references in a root object via an offset.

typedef enum {
    CS_SUCCESS = 0,

    // This is returned when a root with a zero length
    // reference table is attempted to be created.
    CS_EMPTY_ROOT_CREATION,

    // This is returned when an object with a zero lemgth
    // reference table and data array is attempted to be 
    // created.
    CS_EMPTY_OBJECT_CREATION,

    // This is returned when the index of a root is out
    // of bounds for the root set.
    CS_ROOT_INDEX_OUT_OF_BOUNDS,

    // This is returned when the given entry in the root
    // set referenced is not initialized.
    CS_ROOT_INDEX_INVALID,

    // This is returned when the given offset is out
    // of bounds for a specific root.
    CS_ROOT_OFFSET_OUT_OF_BOUNDS,

    // MORE TO COME...
} cs_status_code;

// Add a new root with the given number of references. 
uint64_t cs_new_root(collected_space *cs, uint64_t rt_len);

// This will create a new root object with the exact same references as a
// given existing root. references may be truncated depending on the 
// reference table length given for the new root being created.
uint64_t cs_copy_root(collected_space *cs, uint64_t root_ind, uint64_t rt_len);

// Remove a root object from the root set.
// (It will eventually be garbage collected)
void cs_remove_root(collected_space *cs, uint64_t root_ind);

// Create a new object and store it at a certain offset in a root object. 
void cs_new_obj(collected_space *cs, uint64_t root_ind, uint64_t offset,
        uint64_t rt_len, uint64_t da_size);

// Set a reference to NULL_VADDR.
void cs_null_reference(collected_space *cs, uint64_t root_ind,
        uint64_t dest_offset);

// Move the reference stored at the source offset to the slot
// at destination offset.
// root->rt[dest_offset] = root->rt[src_offset]
void cs_move_reference(collected_space *cs, uint64_t root_ind,
        uint64_t dest_offset, uint64_t src_offset);

// Load a reference from a reference into the root's reference table.
// root->rt[dest_offset] = root->rt[src_offset]->rt[src_int_offset]
void cs_load_reference(collected_space *cs, uint64_t root_ind,
        uint64_t dest_offset, uint64_t src_offset, uint64_t src_rt_offset);

// root->rt[dest_offset]->rt[dest_rt_offset] = root->rt[src_offset]
void cs_store_reference(collected_space *cs, uint64_t root_ind,
        uint64_t dest_offset, uint64_t dest_rt_offset, uint64_t src_offset);

// NOTE: For data reading operations below... buffers cannot be overlaping!
// (i.e. we use memcpy)

// Read len bytes from an objects offset data array, and copy them into buf.
void cs_read_data(collected_space *cs, uint64_t root_ind, 
        uint64_t src_offset, uint64_t src_da_offset, uint64_t len, void *dest);

// Write data to an object's offset data array from the buffer.
void cs_write_data(collected_space *cs, uint64_t root_ind,
        uint64_t dest_offset, uint64_t dest_da_offset, uint64_t len, const void *src);

void cs_print(collected_space *cs);

// Run garbage collection algorithm.
void cs_collect_garbage(collected_space *cs);

// Run try full shift on the underlying memory space.
void cs_try_full_shift(collected_space *cs);

#endif
