#include "./data.h"
#include "../data.h"

#include "../../testing_src/assert.h"

#include <stdio.h>
#include <inttypes.h>

#include "../../core_src/mem.h"

static void test_new_linked_list(int pipe_fd) {
    util_ll *ll = new_linked_list(1, 1);
    delete_linked_list(ll);
}

static const chunit_test LL_NEW_LINKED_LIST = {
    .name = "New Linked List",
    .t = test_new_linked_list,
    .timeout = 5,
};

static void test_ll_next(int pipe_fd) {
    util_ll *ll = new_linked_list(1, sizeof(int64_t));

    *(int64_t *)ll_next(ll) = 10;
    *(int64_t *)ll_next(ll) = -5;

    assert_eq_uint(pipe_fd, 2, ll_len(ll));

    delete_linked_list(ll);
}

static const chunit_test LL_NEXT = {
    .name = "Linked List Next",
    .t = test_ll_next,
    .timeout = 5,
};

static void test_ll_add_and_get(int pipe_fd) {
    util_ll *ll = new_linked_list(1, sizeof(int64_t));

    int64_t v1 = 10, v2 = 5;
    ll_add(ll, &v1);
    ll_add(ll, &v2);

    assert_true(pipe_fd, *(int64_t *)ll_get(ll, 0) == 10);
    assert_true(pipe_fd, *(int64_t *)ll_get(ll, 1) == 5);
    assert_eq_ptr(pipe_fd, NULL, ll_get(ll, 2));

    delete_linked_list(ll);
}

static const chunit_test LL_ADD_AND_GET = {
    .name = "Linked List Add and Get",
    .t = test_ll_add_and_get,
    .timeout = 5,
};

const chunit_test_suite UTIL_TEST_SUITE_LL = {
    .name = "Linked List Suite",
    .tests = {
        &LL_NEW_LINKED_LIST,
        &LL_NEXT,
        &LL_ADD_AND_GET,
    },
    .tests_len = 3
};



