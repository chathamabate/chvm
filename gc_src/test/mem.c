#include "mem.h"
#include "../mem.h"
#include "../../testing_src/assert.h"
#include "../../core_src/io.h"
#include "../../core_src/mem.h"
#include "../../util_src/thread.h"

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

typedef struct {
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
} test_mem_block_context_0;

static void *test_mem_block_worker_0(void *arg) {
    util_thread_spray_context *s_context = arg; 
    test_mem_block_context_0 *c = s_context->context;

    uint64_t *sizes = safe_malloc(1, sizeof(uint64_t) * c->size_mod);

    // Calculate block sizes.
    uint64_t i;
    for (i = 0; i < c->size_mod; i++) {
        sizes[i] = (i + 1) * c->malloc_size_factor;
    }

    addr_book_vaddr *vaddrs = 
        safe_malloc(1, sizeof(addr_book_vaddr) * c->num_mallocs);

    // Malloc a bunch of different blocks with different
    // sizes. (Using the size mod)
    for (i = 0; i < c->num_mallocs; i++) {
        uint64_t min_size = sizes[i % c->size_mod];
        vaddrs[i] = mb_malloc(c->mb, min_size);

        // NOTE: we are assuming our mb is large enough to 
        // take all of our mallocs.
        assert_false(c->tc, null_adb_addr(vaddrs[i]));

        // Here, we take our new piece, and we write the same value
        // into all of its bytes.
        uint8_t *map_iter = adb_get_write(c->adb, vaddrs[i]);
        uint8_t *map_end = map_iter + min_size;

        for (; map_iter < map_end; map_iter++) {
            *map_iter = (uint8_t)(i * s_context->index);
        }

        adb_unlock(c->adb, vaddrs[i]);
    }

    // Now we free using the given mod.
    for (i = 0; i < c->num_mallocs; i += c->free_mod) {
        mb_free(c->mb, vaddrs[i]);
        vaddrs[i] = NULL_VADDR;
    }

    // Finally, we check the remaining vaddrs.
    for (i = 0; i < c->num_mallocs; i++) {
        if (null_adb_addr(vaddrs[i])) {
            continue; // Skip those that were already free.
        }

        uint8_t *map_iter = adb_get_read(c->adb, vaddrs[i]);
        uint8_t *map_end = map_iter + sizes[i % c->size_mod];

        // Now we confirm all bytes remain as expected.
        for (; map_iter < map_end; map_iter++) {
            assert_eq_int(c->tc, (uint8_t)(i * s_context->index), *map_iter);
        }

        adb_unlock(c->adb, vaddrs[i]);

        mb_free(c->mb, vaddrs[i]);
    }

    safe_free(vaddrs);
    safe_free(sizes);

    return NULL;
}

static void test_mem_block_multi_0(chunit_test_context *tc) {
    addr_book *adb = new_addr_book(1, 10);
    mem_block *mb = new_mem_block(1, adb, 30000);

    test_mem_block_context_0 ctx = {
        .tc = tc,
        
        .adb = adb,
        .mb = mb,

        .malloc_size_factor = sizeof(uint64_t),
        .size_mod = 5,
        .free_mod = 2,
        .num_mallocs = 20,
    };

    const uint64_t num_threads = 20;

    util_thread_spray_info *spray = util_thread_spray(1, num_threads, 
                test_mem_block_worker_0, &ctx);

    util_thread_collect(spray);

    delete_mem_block(mb);
    delete_addr_book(adb);
}

const chunit_test MB_MULTI_0 = {
    .name = "Memory Block Multi Threaded 0",
    .t = test_mem_block_multi_0,
    .timeout = 5,
};

static void test_mb_shift_0(chunit_test_context *tc) {
    addr_book *adb = new_addr_book(1, 10);
    mem_block *mb = new_mem_block(1, adb, 1000);

    // With no mallocs yet, there shouldn't be anything
    // to shift.
    assert_false(tc, mb_shift(mb));

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
    mb_shift(mb);

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
    mem_block *mb = new_mem_block(1, adb, 1000);

    const uint64_t size_mod = 3;
    const uint64_t free_mod = 4;
    const uint64_t size_factor = sizeof(int);
    const uint64_t num_mallocs = 20;



    delete_mem_block(mb);
    delete_addr_book(adb);
}

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
        &MB_MULTI_0,
        &MB_SHIFT_0,
        &MB_SHIFT_1,
    },
    .tests_len = 10,
};
