#include "mem.h"
#include "../mem.h"
#include "../../testing_src/assert.h"
#include "../../core_src/io.h"
#include "../../core_src/mem.h"

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

    mb_print(mb);

    delete_mem_block(mb);
    delete_addr_book(adb);
}

static const chunit_test MB_MAF_4 = {
    .name = "Memory Block Malloc/Free 4",
    .t = test_mem_block_maf_4,
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
    },
    .tests_len = 6,
};
