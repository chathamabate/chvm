#include "ms.h"
#include "mb.h"

#include "../ms.h"
#include "../../testing_src/assert.h"
#include "../../testing_src/misc.h"
#include "../../core_src/io.h"
#include "../../util_src/thread.h"
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

    uint8_t vaddr_chnl;

    uint64_t num_mallocs; 
    uint64_t size_factor;
    uint64_t size_mod;
    uint64_t free_mod;
} ms_chop_args;

// Kinda like a hash function here.
static inline uint8_t vaddr_to_unique_byte(addr_book_vaddr vaddr) {
    return (uint8_t)(11 * vaddr.table_index +  13 * vaddr.cell_index);
}

static void ms_chop_and_check(ms_chop_args *args) {
    addr_book_vaddr *vaddrs = 
        safe_malloc(args->vaddr_chnl, sizeof(addr_book_vaddr) * args->num_mallocs);

    uint64_t i;
    for (i = 0; i < args->num_mallocs; i++) {
        uint64_t min_size = args->size_factor * ((i % args->size_mod) + 1);
        addr_book_vaddr v = ms_malloc(args->ms, min_size);

        assert_false(args->tc, null_adb_addr(v));

        vaddrs[i] = v;

        // Write a unique byte into the memory piece given...
        uint8_t ub = vaddr_to_unique_byte(v);
        uint8_t *ptr = ms_get_write(args->ms, v);
        write_test_bytes(ptr, min_size, vaddr_to_unique_byte(v));

        ms_unlock(args->ms, v);    
    }

    for (i = 0; i < args->num_mallocs; i += args->free_mod) {
        ms_free(args->ms, vaddrs[i]);
        vaddrs[i] = NULL_VADDR;
    }

    // Add shift logic here...

    for (i = 0; i < args->num_mallocs; i++) {
        if (null_adb_addr(vaddrs[i])) {
            continue;
        }
        
        uint64_t min_size = 
            args->size_factor * ((i % args->size_mod) + 1);

        // Confirm the expected memory bytes.
        uint8_t ub = vaddr_to_unique_byte(vaddrs[i]);
        uint8_t *ptr = ms_get_read(args->ms, vaddrs[i]);
        check_test_bytes(args->tc, ptr, min_size, ub);

        ms_unlock(args->ms, vaddrs[i]);    
    }

    safe_free(vaddrs);
}

static void test_ms_maf_2(chunit_test_context *tc) {
    mem_space *ms = new_mem_space_seed(1, 1, 10, sizeof(int) * 10);

    ms_chop_args args = {
        .tc = tc,
        .ms = ms,

        .vaddr_chnl = 1,

        .num_mallocs = 50,
        .free_mod = 7,
        .size_factor = sizeof(int),
        .size_mod = 6,
    };

    ms_chop_and_check(&args);

    delete_mem_space(ms);
}

const chunit_test MS_MAF_2 = {
    .name = "Memory Space Malloc and Free 2",
    .t = test_ms_maf_2,
    .timeout = 5,
};


static void *test_ms_worker(void *arg) {
    util_thread_spray_context *s_context = arg;
    ms_chop_args *m_ca = s_context->context;

    ms_chop_and_check(m_ca);

    return NULL;
}


static void test_ms_multi_maf(chunit_test_context *tc) {
    mem_space *ms = new_mem_space_seed(1, 1, 10, sizeof(int) * 30);

    ms_chop_args m_ca = {
        .tc = tc,
        .ms = ms,

        .vaddr_chnl = 1,

        .num_mallocs = 50,
        .free_mod = 7,
        .size_factor = sizeof(int) * 5,
        .size_mod = 6,
    };

    const uint64_t num_threads = 1;

    util_thread_spray_info *spray = 
        util_thread_spray(1, num_threads, test_ms_worker, &m_ca);

    util_thread_collect(spray);

    delete_mem_space(ms);
}

const chunit_test MS_MULTI_MAF = {
    .name = "Memory Space Multi Malloc and Free",
    .t = test_ms_multi_maf,
    .timeout = 5,
};

const chunit_test_suite GC_TEST_SUITE_MS = {
    .name = "Memory Space Test Suite",
    .tests = {
        &MS_NEW,
        &MS_MAF_0,
        &MS_MAF_1,
        &MS_MAF_2,
        &MS_MULTI_MAF,
    },
    .tests_len = 5
};
