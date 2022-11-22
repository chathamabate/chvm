#include "./data.h"
#include "../data.h"

#include "../../testing_src/assert.h"

#include <stdio.h>
#include <inttypes.h>

#include "../../core_src/mem.h"

static uint8_t int64_eq(void *vp1, void *vp2) {
    return *(int64_t *)vp1 == *(int64_t *)vp2;
}

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

static void test_ll_contains(int pipe_fd) {
    util_ll *ll = new_linked_list(1, sizeof(int64_t));

    *(int64_t *)ll_next(ll) = 10;
    *(int64_t *)ll_next(ll) = -5;

    int64_t v1 = 10, v2 = -5;

    assert_true(pipe_fd, ll_contains(ll, &v1, int64_eq));
    assert_true(pipe_fd, ll_contains(ll, &v2, int64_eq));

    int64_t v3 = 4;
    assert_false(pipe_fd, ll_contains(ll, &v3, int64_eq));

    delete_linked_list(ll);
}

static const chunit_test LL_CONTAINS = {
    .name = "Linked List Contains",
    .t = test_ll_contains,
    .timeout = 5
};

static void test_ll_add(int pipe_fd) {
    util_ll *ll = new_linked_list(1, sizeof(int64_t));

    int64_t v1 = 10, v2 = 5;
    ll_add(ll, &v1);
    ll_add(ll, &v2);

    assert_true(pipe_fd, ll_contains(ll, &v1, int64_eq));
    assert_true(pipe_fd, ll_contains(ll, &v2, int64_eq));

    delete_linked_list(ll);
}

static const chunit_test LL_ADD = {
    .name = "Linked List Add",
    .t = test_ll_add,
    .timeout = 5,
};

const chunit_test_suite UTIL_TEST_SUITE_LL = {
    .name = "Linked List Suite",
    .tests = {
        &LL_NEW_LINKED_LIST,
        &LL_NEXT,
        &LL_CONTAINS,
        &LL_ADD,
    },
    .tests_len = 4
};



