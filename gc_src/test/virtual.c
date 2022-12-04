
#include "./virtual.h"
#include "../virtual.h"
#include "../../testing_src/assert.h"

static void test_new_addr_table(int pipe_fd) {
    addr_table *adt = new_addr_table(1, 5);
    delete_addr_table(adt);
}

static const chunit_test ADT_NEW_ADDR_TABLE = {
    .name = "New Address Table",
    .t = test_new_addr_table,
    .timeout = 5,
};

static void test_adt_put_and_get(int pipe_fd) {
    const uint64_t table_len = 4;
    addr_table *adt = new_addr_table(1, table_len);

    uint64_t ptable[table_len];

    uint64_t i;
    for (i = 0; i < table_len; i++) {
        assert_eq_uint(pipe_fd, i, adt_put(adt, ptable + i));
        assert_eq_ptr(pipe_fd, ptable + i, adt_get(adt, i));
    }

    assert_true(pipe_fd, adt_is_full(adt));

    delete_addr_table(adt);
}

static const chunit_test ADT_ADD_AND_GET = {
    .name = "Address Table Put and Get",
    .t = test_adt_put_and_get,
    .timeout = 5,
};

static void test_adt_free(int pipe_fd) {
    const uint64_t table_len = 4;
    addr_table *adt = new_addr_table(1, table_len);

    uint64_t i;
    for (i = 0; i < table_len; i++) {
        adt_put(adt, NULL);
    }

    adt_free(adt, 3);
    adt_free(adt, 0);
    adt_free(adt, 1);

    assert_eq_int(pipe_fd, 1, adt_put(adt, NULL));
    assert_eq_int(pipe_fd, 0, adt_put(adt, NULL));
    assert_eq_int(pipe_fd, 3, adt_put(adt, NULL));

    assert_false(pipe_fd, adt_has_next(adt));

    delete_addr_table(adt);
}

static const chunit_test ADT_FREE = {
    .name = "Address Table Free",
    .t = test_adt_free,
    .timeout = 5,
};

const chunit_test_suite GC_TEST_SUITE_ADT = {
    .name = "Address Table Test Suite",
    .tests = {
        &ADT_NEW_ADDR_TABLE,
        &ADT_ADD_AND_GET,
        &ADT_FREE,
    },
    .tests_len = 3
};



const chunit_test_suite GC_TEST_SUITE_ADB = {
    .name = "Address Book Test Suite",
    .tests = {

    },
    .tests_len = 0
};

