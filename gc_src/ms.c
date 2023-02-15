#include "ms.h"
#include "mb.h"
#include "virt.h"

#include "../core_src/thread.h"
#include "../core_src/mem.h"
#include "../core_src/sys.h"
#include "../core_src/io.h"

#include "../util_src/data.h"

#include <inttypes.h>

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
    if (mb_m_bytes == 0) {
        return NULL;   
    }

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

// When we malloc, we will additionally write the constant address
// of the parent memory block into the allocated cell.
// The memory block doesn't need to know about this.

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

// This assumes the malloc succeeded and is holding the corresponding paddr.
static inline malloc_res ms_interpret_malloc_res(mem_space *ms, mem_block *mb,
        malloc_res res, uint8_t hold) {
    mem_space_malloc_header *ms_mh = res.paddr; 
    ms_mh->mb = mb;

    if (hold) {
        res.paddr = ms_mh + 1;
    } else {
        res.paddr = NULL;
        adb_unlock(ms->adb, res.vaddr);
    }

    return res;
}

// We attempt to malloc into (len / search_divisor) memory blocks.
static const uint64_t SEARCH_DIV = 3;

malloc_res ms_malloc_p(mem_space *ms, uint64_t min_bytes, uint8_t hold) {
    uint64_t padded_bytes = min_bytes + sizeof(mem_space_malloc_header);

    malloc_res res = {
        .vaddr = NULL_VADDR,
        .paddr = NULL,
    };

    if (min_bytes == 0) {
        return res;
    }

    // NOTE: Here comes a nice random algorithm for the boys back at 
    // Rice. (This may make testing a little tricky...)
    
    safe_rdlock(&(ms->mb_list_lck));
    uint64_t num_throws = ms->mb_list_len / SEARCH_DIV;
    safe_rwlock_unlock(&(ms->mb_list_lck));

    // Make sure to try at least one memory block.
    if (num_throws == 0) {
        num_throws = 1;
    }
    
    uint64_t throw, dart;
    mem_block *mb;

    for (throw = 0; throw < num_throws; throw++) {
        safe_rdlock(&(ms->mb_list_lck));
        dart = ms_next_rnd(ms) % ms->mb_list_len;
        mb = ms->mb_list[dart]; 
        safe_rwlock_unlock(&(ms->mb_list_lck));

        res = mb_malloc_and_hold(mb, padded_bytes);

        // Here, our malloc was a success!
        if (!null_adb_addr(res.vaddr)) {
            return ms_interpret_malloc_res(ms, mb, res, hold);
        }
    }

    // This is when we never founnd space for
    // our malloc.
    // Determine if we request a size greater than the default
    // mem block size.
    uint64_t req_bytes = padded_bytes > ms->mb_min_bytes 
        ? padded_bytes : ms->mb_min_bytes;

    mb = new_mem_block(get_chnl(ms), ms->adb, req_bytes);

    // NOTE: this malloc should always work!
    res = mb_malloc_and_hold(mb, padded_bytes);
    res = ms_interpret_malloc_res(ms, mb, res, hold);

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
    
    return res;
}

void ms_free(mem_space *ms, addr_book_vaddr vaddr) {
    mem_block *mb;

    mem_space_malloc_header *ms_mh = adb_get_read(ms->adb, vaddr);
    mb = ms_mh->mb; // Get our corresponding memory block.
    adb_unlock(ms->adb, vaddr);

    mb_free(mb, vaddr);
}

uint8_t ms_allocated(mem_space *ms, addr_book_vaddr vaddr) {
    return adb_allocated(ms->adb, vaddr);
}

void ms_try_full_shift(mem_space *ms) {
    uint64_t len, i;

    safe_rdlock(&(ms->mb_list_lck));
    len = ms->mb_list_len;
    safe_rwlock_unlock(&(ms->mb_list_lck));

    for (i = 0; i < len; i++) {
        safe_rdlock(&(ms->mb_list_lck));
        mem_block *mb = ms->mb_list[i];
        safe_rwlock_unlock(&(ms->mb_list_lck));

        // Don't hold the lock while doing the mb shift...
        // this could potentially take some time.
        mb_try_full_shift(mb);
    }
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

typedef void (*mb_consumer)(mem_block *mb, void *ctx);

static void ms_foreach_mb(mem_space *ms, mb_consumer c, void *ctx) {
    uint64_t len, i;

    safe_rdlock(&(ms->mb_list_lck));
    len = ms->mb_list_len;
    safe_rwlock_unlock(&(ms->mb_list_lck));

    for (i = 0; i < len; i++) {
        safe_rdlock(&(ms->mb_list_lck));
        mem_block *mb = ms->mb_list[i];
        safe_rwlock_unlock(&(ms->mb_list_lck));

        c(mb, ctx);
    }
}

typedef struct {
    adb_cell_consumer c;
    void *og_ctx;
} ms_foreach_mp_consumer_context;

static void ms_foreach_mp_consumer(addr_book_vaddr v, void *paddr, void *ctx) {
    ms_foreach_mp_consumer_context *ms_f_mp_c_ctx = ctx;

    // Skip over malloced header.
    void *new_paddr = (mem_space_malloc_header *)paddr + 1;

    ms_f_mp_c_ctx->c(v, new_paddr, ms_f_mp_c_ctx->og_ctx);
}

void ms_foreach(mem_space *ms, adb_cell_consumer c, void *ctx, uint8_t wr) {
    ms_foreach_mp_consumer_context ms_f_mp_c_ctx = {
        .c = c,
        .og_ctx = ctx,
    };

    adb_foreach(ms->adb, ms_foreach_mp_consumer, &ms_f_mp_c_ctx, wr);
}

typedef struct {
    adb_cell_predicate pred;

    void *og_ctx;
    util_bc *remove_stack; 
} ms_filter_context;

static void ms_foreach_mp_filter(addr_book_vaddr v, void *paddr, void *ctx) {
    ms_filter_context *ms_f_ctx = ctx;

    if (!(ms_f_ctx->pred(v, paddr, ms_f_ctx->og_ctx))) {
        bc_push_back(ms_f_ctx->remove_stack, &v);
    }
}

uint64_t ms_filter(mem_space *ms, adb_cell_predicate pred, void *ctx) {
    util_bc *remove_stack = new_broken_collection(get_chnl(ms), 
            sizeof(addr_book_vaddr), 30, 0);
    
    ms_filter_context ms_f_ctx = {
        .og_ctx = ctx,
        .pred = pred,
        .remove_stack = remove_stack,
    };
    
    ms_foreach(ms, ms_foreach_mp_filter, &ms_f_ctx, 0);

    uint64_t filtered = 0;
    addr_book_vaddr v;
    while (!bc_empty(remove_stack)) {
        bc_pop_back(remove_stack, &v);
        ms_free(ms, v);
        filtered++;
    }

    delete_broken_collection(remove_stack);

    return filtered;
}


static void mp_count_consumer(addr_book_vaddr v, void *paddr, void *ctx) {
    (*(uint64_t *)ctx)++;
}

uint64_t ms_count(mem_space *ms) {
    uint64_t count = 0; 

    ms_foreach(ms, mp_count_consumer, &count, 0);

    return count;
}

void ms_print(mem_space *ms) {
    safe_rdlock(&(ms->mb_list_lck));

    safe_printf("Memory Space : %p : (Len = %" PRIu64 ")\n\n", ms, ms->mb_list_len);

    uint64_t i;
    for (i = 0; i < ms->mb_list_len; i++) {
        safe_printf("Memory Block %" PRIu64 " :\n", i);
        mb_print(ms->mb_list[i]);
    }

    safe_rwlock_unlock(&(ms->mb_list_lck));
}

