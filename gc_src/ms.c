#include "ms.h"
#include "mb.h"
#include "virt.h"

#include "../core_src/thread.h"
#include "../core_src/mem.h"
#include "../core_src/sys.h"

// For sorting... we want a linked list!
struct mem_space_struct {
    addr_book * const adb; 
    const uint64_t mb_min_bytes;

    // Classic arraylist construction
    // below for mb_list. 

    // Data used for simple thread safe pseudo random number
    // generation. May want to take this out of here at some point.
    // Also nice for custom rng tho...
    pthread_mutex_t rnd_lck;
    uint64_t seed;

    pthread_rwlock_t mb_list_lck;

    uint64_t mb_list_len;
    uint64_t mb_list_cap;

    // NOTE: this never ever ever shrinks!
    mem_block **mb_list;
};

mem_space *new_mem_space_seed(uint64_t chnl, uint64_t seed, 
        uint64_t adb_t_cap, uint64_t mb_m_bytes) {
    mem_space *ms = safe_malloc(chnl, sizeof(mem_space));

    *(addr_book **)&(ms->adb) = new_addr_book(chnl, adb_t_cap);;
    *(uint64_t *)&(ms->mb_min_bytes) = mb_m_bytes;

    safe_mutex_init(&(ms->rnd_lck), NULL);
    ms->seed = seed;

    // Create our memory space with one single empty memory block.
    safe_rwlock_init(&(ms->mb_list_lck), NULL);

    ms->mb_list_cap = 2;
    ms->mb_list = safe_malloc(chnl, sizeof(mem_block *) * ms->mb_list_cap);   

    ms->mb_list_len = 1;
    ms->mb_list[0] = new_mem_block(chnl, ms->adb, mb_m_bytes);

    return ms;
}

void delete_mem_space(mem_space *ms) {
    // Not gonna delete the adb as it was given to us!

    // Again, this is just for consistency.
    // delete mem space should never be called in parallel
    // with any other calls to the given ms. 
    safe_wrlock(&(ms->mb_list_lck));

    uint64_t i;
    for (i = 0; i < ms->mb_list_len; i++) {
        delete_mem_block(ms->mb_list[i]);
    }

    safe_free(ms->mb_list);

    ms->mb_list_cap = 0;
    ms->mb_list_len = 0;
    ms->mb_list = NULL;

    safe_rwlock_unlock(&(ms->mb_list_lck));

    // Must do this after deleting blocks.
    delete_addr_book(ms->adb);

    safe_rwlock_destroy(&(ms->mb_list_lck));
    safe_mutex_destroy(&(ms->rnd_lck));

    // finally, delete the memory space itself.
    safe_free(ms);
}

// This is a constant, needs no lock.
addr_book *ms_get_adb(mem_space *ms) {
    return ms->adb;
}

// When we malloc, we will additionally write the constant address
// of the parent memory block into the allocated cell.
// The memory block doesn't need to know about this.

typedef mem_block *mem_space_malloc_header;

static inline uint64_t ms_next_rnd(mem_space *ms) {
    uint64_t curr_seed;

    safe_mutex_lock(&(ms->rnd_lck));
    ms->seed++;
    
    if (ms->seed == 0) {
        ms->seed = 1;
    }

    curr_seed = ms->seed;
    safe_mutex_unlock(&(ms->rnd_lck));

    // NOTE: Here is my very simple algorithm.
    // Hopefully one day I could read more on how
    // to make this more effective.
    
    uint64_t a = curr_seed * 15485863;
    uint64_t b = curr_seed * 2038074743;

    return (a * a * a) + (b * b);
}

// We attempt to malloc into (len / search_divisor) memory blocks.
static const uint64_t SEARCH_DIV = 3;

addr_book_vaddr ms_malloc(mem_space *ms, uint64_t min_bytes) {
    uint64_t padded_bytes = min_bytes + sizeof(mem_space_malloc_header);

    addr_book_vaddr res = NULL_VADDR;

    // NOTE: Here comes a nice random algorithm for the boys back at 
    // Rice. (This may make testing a little tricky...)
    
    safe_rdlock(&(ms->mb_list_lck));

    uint64_t search_len = ms->mb_list_len / SEARCH_DIV;

    // Make sure to try at least one memory block.
    if (search_len == 0) {
        search_len = 1;
    }
    
    // Random start index.
    uint64_t i = ms_next_rnd(ms) % ms->mb_list_len;
    mem_block *mb;

    uint64_t moves;

    for (moves = 0; moves < search_len; moves++) {
        mb = ms->mb_list[i]; 
        res = mb_malloc(mb, padded_bytes);

        // Here, our malloc was a success!
        if (!null_adb_addr(res)) {
            break;
        }

        // Cyclically increment i.
        i = (i + 1) % ms->mb_list_len;
    }

    safe_rwlock_unlock(&(ms->mb_list_lck));

    // This is when we never founnd space for
    // our malloc.
    if (null_adb_addr(res)) {
        // Determine if we request a size greater than the default
        // mem block size.
        uint64_t req_bytes = padded_bytes > ms->mb_min_bytes 
            ? padded_bytes : ms->mb_min_bytes;

        mb = new_mem_block(get_chnl(ms), ms->adb, req_bytes);

        // NOTE: this malloc should always work!
        res = mb_malloc(mb, padded_bytes); 

        // Finally, after our successful malloc, add mb to the mb_list.
        safe_wrlock(&(ms->mb_list_lck));

        if (ms->mb_list_len == ms->mb_list_cap) {
            // NOTE: One day we may want to check for overflow...
            // However, I think we'd run out of memory before this occurs.
            ms->mb_list_cap *= 2;
            ms->mb_list = safe_realloc(ms->mb_list, sizeof(mem_block *) * ms->mb_list_cap);
        }

        // Add our memory block to the end of the list.
        ms->mb_list[(ms->mb_list_len)++] = mb;
        
        safe_rwlock_unlock(&(ms->mb_list_lck));
    } 

    mem_space_malloc_header *ms_mh = adb_get_write(ms->adb, res);
    *ms_mh = mb; // Place our mem block pointer at the beginning of our 
                 // allocated memory piece.
    adb_unlock(ms->adb, res);
    
    return res;
}

void ms_free(mem_space *ms, addr_book_vaddr vaddr) {
    mem_block *mb;

    mem_space_malloc_header *ms_mh = adb_get_read(ms->adb, vaddr);
    mb = *ms_mh; // Get our corresponding memory block.
    adb_unlock(ms->adb, vaddr);
    
    mb_free(mb, vaddr);
}

void *ms_get_write(mem_space *ms, addr_book_vaddr vaddr) {
    return (mem_space_malloc_header *)adb_get_write(ms->adb, vaddr) + 1;
}

void *ms_get_read(mem_space *ms,addr_book_vaddr vaddr) {
    return (mem_space_malloc_header *)adb_get_read(ms->adb, vaddr) + 1;
}

void ms_unlock(mem_space *ms,addr_book_vaddr vaddr) {
    adb_unlock(ms->adb, vaddr);
}


