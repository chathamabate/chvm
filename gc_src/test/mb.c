#include "mb.h"
#include "../mb.h"
#include "../../testing_src/assert.h"
#include "../../core_src/io.h"
#include "../../core_src/mem.h"
#include "../../util_src/thread.h"

// Kinda like a hash function here.
static inline uint8_t vaddr_to_unique_byte(addr_book_vaddr vaddr) {
    return (uint8_t)(11 * vaddr.table_index +  13 * vaddr.cell_index);
}

static void fill_unique(addr_book *adb, addr_book_vaddr vaddr, 
        uint64_t min_size) {
    // We can change this method however we see fit.
    uint8_t fill_byte = vaddr_to_unique_byte(vaddr);

    uint8_t *iter = adb_get_write(adb, vaddr);
    uint8_t *end = iter + min_size;

    for (; iter < end; iter++) {
        *iter = fill_byte;
    }

    adb_unlock(adb, vaddr);
}

static void check_unique_vaddr_body(chunit_test_context *tc, 
        addr_book *adb, addr_book_vaddr vaddr, uint64_t min_size) {
    uint64_t expected_data = (uint64_t)vaddr_to_unique_byte(vaddr);

    uint8_t *iter = adb_get_read(adb, vaddr);
    uint8_t *end = iter + min_size;

    for (; iter < end; iter++) {
        uint64_t found_data = (uint64_t)(*iter);
        assert_eq_uint(tc, expected_data, found_data);
    }

    adb_unlock(adb, vaddr);
}

static void test_new_mem_block(chunit_test_context *tc) {
    addr_book *adb = new_addr_book(1, 5);
    mem_block *mb = new_mem_block(1, adb, 10); 

    delete_mem_block(mb);
    delete_addr_book(adb);
} 

static const chunit_test MB_NEW = {
    .name = "New Memory Block",
    .t = test_new_mem_block,
    .timeout = 5,
};

static void test_mem_block_maf_0(chunit_test_context *tc) {
    addr_book *adb = new_addr_book(1, 5);
    mem_block *mb = new_mem_block(1, adb, sizeof(int)); 

    addr_book_vaddr vaddr = mb_malloc(mb, sizeof(int));

    assert_false(tc, null_adb_addr(vaddr));

    mb_free(mb, vaddr);

    delete_mem_block(mb);
    delete_addr_book(adb);
}

static const chunit_test MB_MAF_0 = {
    .name = "Memory Block Malloc/Free 0",
    .t = test_mem_block_maf_0,
    .timeout = 5,
};

static void test_mem_block_maf_1(chunit_test_context *tc) {
    addr_book *adb = new_addr_book(1, 5);
    mem_block *mb = new_mem_block(1, adb, 100); 

    // Try just mallocing a bunch of times in a row.
    while (!null_adb_addr(mb_malloc(mb, sizeof(int))));

    delete_mem_block(mb);
    delete_addr_book(adb);
}

static const chunit_test MB_MAF_1 = {
    .name = "Memory Block Malloc/Free 1",
    .t = test_mem_block_maf_1,
    .timeout = 5,
};

static void test_mem_block_maf_2(chunit_test_context *tc) {
    addr_book *adb = new_addr_book(1, 5);
    mem_block *mb = new_mem_block(1, adb, 100); 

    // Now let's try some mallocs and frees 
    // in a row.
    
    addr_book_vaddr v1 = mb_malloc(mb, sizeof(int));
    assert_false(tc, null_adb_addr(v1));

    addr_book_vaddr v2 = mb_malloc(mb, sizeof(int));
    assert_false(tc, null_adb_addr(v2));

    mb_free(mb, v1);

    addr_book_vaddr v3 = mb_malloc(mb, sizeof(int));
    assert_false(tc, null_adb_addr(v3));

    delete_mem_block(mb);
    delete_addr_book(adb);
}

static const chunit_test MB_MAF_2 = {
    .name = "Memory Block Malloc/Free 2",
    .t = test_mem_block_maf_2,
    .timeout = 5,
};

static void test_mem_block_maf_3(chunit_test_context *tc) {
    const uint64_t block_min_size = 1000;

    addr_book *adb = new_addr_book(1, 5);
    mem_block *mb = new_mem_block(1, adb, block_min_size); 

    // Now we need to test coalescing.
    
    const uint64_t num_sizes = 3;
    const uint64_t vaddrs_len = 6;
    addr_book_vaddr vaddrs[vaddrs_len];

    uint64_t i;
    for (i = 0; i < vaddrs_len; i++) {
        // Slightly different sized blocks here.
        vaddrs[i] = mb_malloc(mb, ((i % num_sizes) + 1) * 20);

        assert_false(tc, null_adb_addr(vaddrs[i]));
    }

    // Testing freeing in random order. 
    
    mb_free(mb, vaddrs[5]);
    mb_free(mb, vaddrs[3]);
    mb_free(mb, vaddrs[2]);
    mb_free(mb, vaddrs[4]);
    mb_free(mb, vaddrs[1]);
    mb_free(mb, vaddrs[0]);

    addr_book_vaddr final_vaddr = mb_malloc(mb, block_min_size);
    assert_false(tc, null_adb_addr(final_vaddr));

    delete_mem_block(mb);
    delete_addr_book(adb);
}

static const chunit_test MB_MAF_3 = {
    .name = "Memory Block Malloc/Free 3",
    .t = test_mem_block_maf_3,
    .timeout = 5,
};

static void test_mem_block_maf_4(chunit_test_context *tc) {
    addr_book *adb = new_addr_book(1, 5);
    mem_block *mb = new_mem_block(1, adb, 2000); 

    const uint64_t num_sizes = 5;
    const uint64_t vaddrs_len = 20;
    addr_book_vaddr vaddrs[vaddrs_len];

    uint64_t i;
    for (i = 0; i < vaddrs_len; i++) {
        vaddrs[i] = mb_malloc(mb, ((i % num_sizes) + 1) * 15);
        assert_false(tc, null_adb_addr(vaddrs[i]));
    }

    for (i = 0; i < vaddrs_len; i+=2) {
        mb_free(mb, vaddrs[i]); 
    }

    while (!null_adb_addr(mb_malloc(mb, 32)));

    delete_mem_block(mb);
    delete_addr_book(adb);
}

static const chunit_test MB_MAF_4 = {
    .name = "Memory Block Malloc/Free 4",
    .t = test_mem_block_maf_4,
    .timeout = 5,
};

static void test_mem_block_write(chunit_test_context *tc) {
    addr_book *adb = new_addr_book(1, 5);
    mem_block *mb = new_mem_block(1, adb, 2000); 

    const uint64_t vaddrs_len = 20;
    addr_book_vaddr vaddrs[vaddrs_len];

    uint64_t i;
    for (i = 0; i < vaddrs_len; i++) {
        vaddrs[i] = mb_malloc(mb, sizeof(int));
        assert_false(tc, null_adb_addr(vaddrs[i]));

        // Store index in each block.
        int *paddr = (int *)adb_get_write(adb, vaddrs[i]); 
        *paddr = i;
        adb_unlock(adb, vaddrs[i]);
    }

    // Some random addresses to free.
    const uint64_t num_frees = 10;
    const uint64_t frees[num_frees] = {
        3, 4,
        7, 8,
        5, 10,
        13, 17, 
        15, 16,
    };

    for (i = 0; i < num_frees; i++) {
        mb_free(mb, vaddrs[frees[i]]);
        vaddrs[frees[i]] = NULL_VADDR;
    }

    for (i = 0; i < vaddrs_len; i++) {
        if (null_adb_addr(vaddrs[i])) {
            continue;
        }

        int *paddr = (int *)adb_get_read(adb, vaddrs[i]);
        int res = *paddr;
        adb_unlock(adb, vaddrs[i]);

        assert_eq_int(tc, i, res);
    }

    delete_mem_block(mb);
    delete_addr_book(adb);
}

const chunit_test MB_WRITE = {
    .name = "Memory Block Write",
    .t = test_mem_block_write,
    .timeout = 5,
};

// These arguments are used to chop up the inside
// of a memory block in a way which is useful for
// testing different behavoirs of the memory block.
typedef struct {
    // Chopping will always include testing.
    chunit_test_context *tc;

    mem_block *mb;
    addr_book *adb;

    // Number of mallocs to perform.
    uint64_t num_mallocs;
    uint64_t malloc_size_factor;

    // How to size blocks.
    uint64_t size_mod;

    // How to free blocks.
    uint64_t free_mod;

    // Whether or not data should be shifted 
    // after free occurs.
    uint8_t shift_test;

    // Channel for mallocs.
    uint8_t malloc_chnl;
} chop_args;

static void mb_chop_and_test(chop_args *ca) {
    uint64_t *sizes = safe_malloc(ca->malloc_chnl, 
            sizeof(uint64_t) * ca->size_mod);

    uint64_t i, min_size;

    for (i = 0; i < ca->size_mod; i++) {
        sizes[i] = (i + 1) *ca->malloc_size_factor;
    }

    addr_book_vaddr *vaddrs = safe_malloc(ca->malloc_chnl, 
            sizeof(addr_book_vaddr) * ca->num_mallocs);

    for (i = 0; i < ca->num_mallocs; i++) {
        min_size = sizes[i % ca->size_mod];
        vaddrs[i] = mb_malloc(ca->mb, min_size);

        assert_false(ca->tc, null_adb_addr(vaddrs[i]));
        fill_unique(ca->adb, vaddrs[i], min_size);
    }

    for (i = 0; i < ca->num_mallocs; i += ca->free_mod) {
        mb_free(ca->mb, vaddrs[i]);
        vaddrs[i] = NULL_VADDR;
    }

    // safe_printf("\nAfter Free\n");
    // mb_print(ca->mb);

    if (ca->shift_test) {
        mb_try_full_shift(ca->mb); 

        // safe_printf("\nAfter Shifts\n");
        // mb_print(ca->mb);
    }

    for (i = 0; i < ca->num_mallocs; i++) {
        if (null_adb_addr(vaddrs[i])) {
            continue;
        }

        min_size = sizes[i % ca->size_mod];
        check_unique_vaddr_body(ca->tc, ca->adb, vaddrs[i], min_size);
    }

    safe_free(vaddrs);
    safe_free(sizes);
}

static void test_mb_shift_0(chunit_test_context *tc) {
    addr_book *adb = new_addr_book(1, 10);
    mem_block *mb = new_mem_block(1, adb, 1000);

    // With no mallocs yet, there shouldn't be anything
    // to shift.
    assert_eq_uint(tc, MB_NOT_NEEDED, mb_try_shift(mb));

    delete_mem_block(mb);
    delete_addr_book(adb);
}

const chunit_test MB_SHIFT_0 = {
    .name = "Memory Block Shift 0",
    .t = test_mb_shift_0,
    .timeout = 5,
};

static void test_mb_shift_1(chunit_test_context *tc) {
    addr_book *adb = new_addr_book(1, 10);
    mem_block *mb = new_mem_block(1, adb, 1000);

    addr_book_vaddr vaddr1 = mb_malloc(mb, sizeof(int));
    addr_book_vaddr vaddr2 = mb_malloc(mb, sizeof(int));

    int *paddr = adb_get_write(adb, vaddr2);
    *paddr = -5;
    adb_unlock(adb, vaddr2);

    mb_free(mb, vaddr1);
    mb_try_shift(mb);
    
    paddr = adb_get_read(adb, vaddr2);
    assert_eq_int(tc, -5, *paddr);
    adb_unlock(adb, vaddr2);

    delete_mem_block(mb);
    delete_addr_book(adb);
}

const chunit_test MB_SHIFT_1 = {
    .name = "Memory Block Shift 1",
    .t = test_mb_shift_1,
    .timeout = 5,
};

static void test_mb_shift_2(chunit_test_context *tc) {
    addr_book *adb = new_addr_book(1, 10);
    mem_block *mb = new_mem_block(1, adb, 2000);
     
    chop_args ca = {
        .tc = tc,

        .adb = adb,
        .mb = mb,

        .free_mod = 4,
        .size_mod = 5,
        .num_mallocs = 23,

        .malloc_size_factor = sizeof(uint64_t),
        .malloc_chnl = 1,
        .shift_test = 1, // Should shift.
    };

    mb_chop_and_test(&ca);

    delete_mem_block(mb);
    delete_addr_book(adb);
}

const chunit_test MB_SHIFT_2 = {
    .name = "Memory Block Shift 2",
    .t = test_mb_shift_2,
    .timeout = 5,
};

static void test_mb_shift_3(chunit_test_context *tc)  {
    addr_book *adb = new_addr_book(1, 10);
    mem_block *mb = new_mem_block(1, adb, 2000);

    addr_book_vaddr v1 = mb_malloc(mb, 10);
    addr_book_vaddr v2 = mb_malloc(mb, 10);

    mb_free(mb, v1);

    adb_get_write(adb, v2);

    // While it is not truly guaranteed,
    // we would hope the block pointed to by
    // v2 is in the middle of two free blocks.
    //
    // Thus, this would try to shift, but halt!
    // Returning MB_BUSY.
    // This should result in a deadlock.
    mb_shift_res res = mb_try_shift(mb);

    adb_unlock(adb, v2);

    delete_mem_block(mb);
    delete_addr_book(adb);
}

const chunit_test MB_SHIFT_3 = {
    .name = "Memory Block Shift 3",
    .t = test_mb_shift_3,
    .timeout = 5,
};

static void test_mb_shift_4(chunit_test_context *tc)  {
    addr_book *adb = new_addr_book(1, 10);
    mem_block *mb = new_mem_block(1, adb, 2000);

    const uint64_t num_mallocs = 20;
    const uint64_t lock_mod = 3;
    const uint64_t free_mod = 4;

    addr_book_vaddr vaddrs[num_mallocs];

    uint64_t i;
    for (i = 0; i < num_mallocs; i++) {
        vaddrs[i] = mb_malloc(mb, 10);
    }

    // Here we free certain pieces, and lock on others.
    // Just making sure try full shift doesn't block!

    for (i = 0; i < num_mallocs; i += free_mod) {
        mb_free(mb, vaddrs[i]);
    } 

    for (i = 0; i < num_mallocs; i += lock_mod) {
        if (i % free_mod) {
            adb_get_write(adb, vaddrs[i]);
        }
    }

    mb_try_full_shift(mb);

    // This should cause a deadlock.
    // mb_full_shift(mb);

    for (i = 0; i < num_mallocs; i += lock_mod) {
        if (i % free_mod) {
            adb_unlock(adb, vaddrs[i]);
        }
    }

    delete_mem_block(mb);
    delete_addr_book(adb);
}

const chunit_test MB_SHIFT_4 = {
    .name = "Memory Block Shift 4",
    .t = test_mb_shift_4,
    .timeout = 5,
};

static void test_mb_shift_5(chunit_test_context *tc)  {
    addr_book *adb = new_addr_book(1, 10);
    mem_block *mb = new_mem_block(1, adb, 2000);

    addr_book_vaddr vaddrs[4] =  {
        mb_malloc(mb, 10),
        mb_malloc(mb, 10),
        mb_malloc(mb, 10),
        mb_malloc(mb, 10),
    };

    mb_free(mb, vaddrs[0]);
    mb_free(mb, vaddrs[2]);

    adb_get_read(adb, vaddrs[1]);

    // Two shifts in a row may cause a deadlock!
    // mb_shift(mb);
    // mb_shift(mb);
    
    // These won't though!
    mb_try_shift(mb);
    mb_try_shift(mb);

    adb_unlock(adb, vaddrs[1]);

    delete_mem_block(mb);
    delete_addr_book(adb);
}

const chunit_test MB_SHIFT_5 = {
    .name = "Memory Block Shift 5",
    .t = test_mb_shift_5,
    .timeout = 5,
};

static void *test_mem_block_worker(void *arg) {
    util_thread_spray_context *s_context = arg; 
    chop_args *ca = s_context->context;

    mb_chop_and_test(ca);

    return NULL;
}

static void test_mem_block_multi_0(chunit_test_context *tc) {
    addr_book *adb = new_addr_book(1, 10);
    mem_block *mb = new_mem_block(1, adb, 30000);

    chop_args ca = {
        .tc = tc,

        .adb = adb,
        .mb = mb,

        .malloc_size_factor = sizeof(uint64_t),
        .size_mod = 5,
        .free_mod = 2,
        .num_mallocs = 20,

        .shift_test = 0,
        .malloc_chnl = 1,
    };

    const uint64_t num_threads = 10;

    util_thread_spray_info *spray = util_thread_spray(1, num_threads, 
                test_mem_block_worker, &ca);

    util_thread_collect(spray);

    delete_mem_block(mb);
    delete_addr_book(adb);
}

const chunit_test MB_MULTI_0 = {
    .name = "Memory Block Multi Threaded 0",
    .t = test_mem_block_multi_0,
    .timeout = 5,
};

// Same as above multithreaded test, but with shifting.
// (And different chop arguments)
static void test_mem_block_multi_1(chunit_test_context *tc) {
    addr_book *adb = new_addr_book(1, 32);
    mem_block *mb = new_mem_block(1, adb, 25000);

    chop_args ca = {
        .tc = tc,

        .adb = adb,
        .mb = mb,

        .malloc_size_factor = sizeof(uint64_t),
        .size_mod = 5,
        .free_mod = 4,
        .num_mallocs = 20,

        .shift_test = 1, // Should include a shift here.
        .malloc_chnl = 1,
    };

    const uint64_t num_threads = 20;

    util_thread_spray_info *spray = util_thread_spray(1, num_threads, 
                test_mem_block_worker, &ca);

    util_thread_collect(spray);

    delete_mem_block(mb);
    delete_addr_book(adb);
}

const chunit_test MB_MULTI_1 = {
    .name = "Memory Block Multi Threaded 1",
    .t = test_mem_block_multi_1,
    .timeout = 5,
};

const chunit_test_suite GC_TEST_SUITE_MB = {
    .name = "Memory Block Test Suite",
    .tests = {
        &MB_NEW,
        &MB_MAF_0,
        &MB_MAF_1,
        &MB_MAF_2,
        &MB_MAF_3,

        &MB_MAF_4,
        &MB_WRITE,
        &MB_SHIFT_0,
        &MB_SHIFT_1,
        &MB_SHIFT_2,

        &MB_SHIFT_3,
        &MB_SHIFT_4,
        &MB_SHIFT_5, 
        &MB_MULTI_0,
        &MB_MULTI_1,
    },
    .tests_len = 15,
};
