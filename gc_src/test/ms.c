#include "ms.h"
#include "../ms.h"
#include "../../testing_src/assert.h"
#include <string.h>

static void test_new_mem_space(chunit_test_context *tc) {
    mem_space *ms = new_mem_space_seed(1, 1, 10, 10);
    delete_mem_space(ms);
}

const chunit_test MS_NEW = {
    .name = "New Memory Space",
    .t = test_new_mem_space,
    .timeout = 5,
};

static void test_ms_maf_0(chunit_test_context *tc) {
    mem_space *ms = new_mem_space_seed(1, 1, 10, sizeof(int));

    addr_book_vaddr vaddr = ms_malloc(ms, sizeof(int));

    int *paddr = ms_get_write(ms, vaddr);
    *paddr = 4;
    ms_unlock(ms, vaddr);

    paddr = ms_get_read(ms, vaddr);
    int integer = *paddr;
    ms_unlock(ms, vaddr);

    assert_eq_int(tc, 4, integer);

    delete_mem_space(ms);
}

const chunit_test MS_MAF_0 = {
    .name = "Memory Space Malloc and Free 0",
    .t = test_ms_maf_0,
    .timeout = 5,
};

static void test_ms_maf_1(chunit_test_context *tc) {
    const char *msg = "Hello";

    mem_space *ms = new_mem_space_seed(1, 1, 10, sizeof(int));
    addr_book_vaddr vaddr = ms_malloc(ms, strlen(msg) + 1);

    char *paddr = ms_get_write(ms, vaddr);
    strcpy(paddr, msg);
    ms_unlock(ms, vaddr);

    paddr = ms_get_read(ms, vaddr);
    assert_eq_str(tc, msg, paddr);
    ms_unlock(ms, vaddr);

    delete_mem_space(ms);
}

const chunit_test MS_MAF_1 = {
    .name = "Memory Space Malloc and Free 1",
    .t = test_ms_maf_1,
    .timeout = 5,
};

typedef struct {
    chunit_test_context *tc;
    mem_space *ms;

    uint64_t num_mallocs; 
    uint64_t size_factor;
    uint64_t size_mod;
    uint64_t free_mod;
} ms_chop_args;

// TODO : finish this up here... shouldn't be too much testing tbh.

const chunit_test_suite GC_TEST_SUITE_MS = {
    .name = "Memory Space Test Suite",
    .tests = {
        &MS_NEW,
        &MS_MAF_0,
        &MS_MAF_1,
    },
    .tests_len = 3
};
