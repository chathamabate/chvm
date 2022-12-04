
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

static void test_adt_next_and_has_next(int pipe_fd) {
    const uint64_t table_len = 4;
    addr_table *adt = new_addr_table(1, table_len);

    assert_true(pipe_fd, adt_has_next(adt));

    uint64_t i;
    for (i = 0; i < table_len; i++) {
        assert_eq_uint(pipe_fd, i, adt_next(adt));
    }

    assert_false(pipe_fd, adt_has_next(adt));

    delete_addr_table(adt);
}

static const chunit_test ADT_NEXT_AND_HAS_NEXT = {
    .name = "Address Table Next and Has Next",
    .t = test_adt_next_and_has_next,
    .timeout = 5,
};

static void test_adt_free(int pipe_fd) {
    const uint64_t table_len = 4;
    addr_table *adt = new_addr_table(1, table_len);

    uint64_t i;
    for (i = 0; i < table_len; i++) {
        adt_next(adt);
    }

    adt_free(adt, 3);
    adt_free(adt, 0);
    adt_free(adt, 1);

    assert_eq_int(pipe_fd, 1, adt_next(adt));
    assert_eq_int(pipe_fd, 0, adt_next(adt));
    assert_eq_int(pipe_fd, 3, adt_next(adt));

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
        &ADT_NEXT_AND_HAS_NEXT,
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

