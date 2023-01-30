#include "./data.h"
#include "../data.h"

#include "../../testing_src/assert.h"

#include <stdio.h>
#include <inttypes.h>

#include "../../core_src/io.h"
#include "../../core_src/mem.h"

static void test_new_linked_list(chunit_test_context *tc) {
    util_ll *ll = new_linked_list(1, 1);
    delete_linked_list(ll);
}

static const chunit_test LL_NEW_LINKED_LIST = {
    .name = "New Linked List",
    .t = test_new_linked_list,
    .timeout = 5,
};

static void test_ll_next(chunit_test_context *tc) {
    util_ll *ll = new_linked_list(1, sizeof(int64_t));

    *(int64_t *)ll_next(ll) = 10;
    *(int64_t *)ll_next(ll) = -5;

    assert_eq_uint(tc, 2, ll_len(ll));

    delete_linked_list(ll);
}

static const chunit_test LL_NEXT = {
    .name = "Linked List Next",
    .t = test_ll_next,
    .timeout = 5,
};

static void test_ll_add_and_get(chunit_test_context *tc) {
    util_ll *ll = new_linked_list(1, sizeof(int64_t));

    int64_t v1 = 10, v2 = 5;
    ll_add(ll, &v1);
    ll_add(ll, &v2);

    assert_true(tc, *(int64_t *)ll_get(ll, 0) == 10);
    assert_true(tc, *(int64_t *)ll_get(ll, 1) == 5);
    assert_eq_ptr(tc, NULL, ll_get(ll, 2));

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

static void cell_print_uint(void *cell, void *ctx) {
    safe_printf(" %" PRIu64, *(uint64_t *)cell);
}

static inline void bc_print_uint(util_bc *bc) {
    safe_printf("BC:");
    bc_foreach(bc, cell_print_uint, NULL);
    safe_printf("\n");
}

static void test_new_broken_collection(chunit_test_context *tc) {
    util_bc *bc = new_broken_collection(1, sizeof(uint64_t), 4, 0);
    delete_broken_collection(bc);
}

static const chunit_test BC_NEW = {
    .name = "New Broken Collection",
    .timeout = 5,
    .t = test_new_broken_collection,
};

static inline void bc_push_seq(util_bc *bc, uint64_t pushes, uint8_t front) {
    uint64_t i;
    for (i = 0; i < pushes; i++) {
        if (front) {
            bc_push_front(bc, &i);
        } else {
            bc_push_back(bc, &i);
        }
    }
}

static void test_bc_push_and_pop(chunit_test_context *tc, util_bc *bc, 
        uint64_t pushes, uint8_t front) {
    bc_push_seq(bc, pushes, front);

    uint64_t i, res;
    for (i = 0; i < pushes; i++) {
        if (front) {
            bc_pop_front(bc, &res);
        } else {
            bc_pop_back(bc, &res); 
        }

        assert_eq_uint(tc, pushes - 1 - i, res);
    }

    assert_eq_uint(tc, 0, bc_len(bc));
}

static void test_bc_back(chunit_test_context *tc) {
    util_bc *bc = new_broken_collection(1, sizeof(uint64_t), 4, 0);
    test_bc_push_and_pop(tc, bc, 17, 0);
    delete_broken_collection(bc);
}

static const chunit_test BC_BACK = {
    .name = "Broken Collection Back",
    .timeout = 5,
    .t = test_bc_back,
};

static void test_bc_back_del(chunit_test_context *tc) {
    util_bc *bc = new_broken_collection(1, sizeof(uint64_t), 3, 1);
    test_bc_push_and_pop(tc, bc, 25, 0);
    assert_eq_uint(tc, 1, bc_get_num_tables(bc));
    delete_broken_collection(bc);
}

static const chunit_test BC_BACK_DEL = {
    .name = "Broken Collection Back with Deletion",
    .timeout = 5,
    .t = test_bc_back_del,
};

static void test_bc_front(chunit_test_context *tc) {
    util_bc *bc = new_broken_collection(1, sizeof(uint64_t), 7, 0);
    test_bc_push_and_pop(tc, bc, 23, 1);
    delete_broken_collection(bc);
}

static const chunit_test BC_FRONT = {
    .name = "Broken Collection Front",
    .timeout = 5,
    .t = test_bc_front,
};

static void test_bc_front_del(chunit_test_context *tc) {
    util_bc *bc = new_broken_collection(1, sizeof(uint64_t), 9, 1);
    test_bc_push_and_pop(tc, bc, 33, 1);
    assert_eq_uint(tc, 1, bc_get_num_tables(bc));
    delete_broken_collection(bc);
}

static const chunit_test BC_FRONT_DEL = {
    .name = "Broken Collection Front with Deletion",
    .timeout = 5,
    .t = test_bc_front_del,
};

static void test_bc_queue_p(chunit_test_context *tc, util_bc *bc, 
        uint64_t iters, uint64_t buffer_len, uint8_t front) {
    bc_push_seq(bc, buffer_len, front);
     
    uint64_t iter, i, res;
    for (iter = 0; iter < iters; iter++) {
        bc_push_seq(bc, buffer_len, front);

        for (i = 0; i < buffer_len; i++) {
            if (front) {
                bc_pop_back(bc, &res);
            } else {
                bc_pop_front(bc, &res);
            }

            // Note we are queueing here!
            assert_eq_uint(tc, i, res);
        } 
    } 

    assert_eq_uint(tc, buffer_len, bc_len(bc));
}

static void test_bc_queue_back(chunit_test_context *tc) {
    util_bc *bc = new_broken_collection(1, sizeof(uint64_t), 3, 0);
    test_bc_queue_p(tc, bc, 30, 5, 0);
    delete_broken_collection(bc);
}

static const chunit_test BC_QUEUE_BACK = {
    .name = "Broken Collection Queue Back",
    .t = test_bc_queue_back,
    .timeout = 5,
};

static void test_bc_queue_back_del(chunit_test_context *tc) {
    util_bc *bc = new_broken_collection(1, sizeof(uint64_t), 4, 1);
    test_bc_queue_p(tc, bc, 30, 5, 0);
    delete_broken_collection(bc);
}

static const chunit_test BC_QUEUE_BACK_DEL = {
    .name = "Broken Collection Queue Back with Deletion",
    .t = test_bc_queue_back_del,
    .timeout = 5,
};

static void test_bc_queue_front(chunit_test_context *tc) {
    util_bc *bc = new_broken_collection(1, sizeof(uint64_t), 6, 0);
    test_bc_queue_p(tc, bc, 30, 5, 1);
    delete_broken_collection(bc);
}

static const chunit_test BC_QUEUE_FRONT = {
    .name = "Broken Collection Queue Front",
    .t = test_bc_queue_front,
    .timeout = 5,
};

static void test_bc_queue_front_del(chunit_test_context *tc) {
    util_bc *bc = new_broken_collection(1, sizeof(uint64_t), 3, 1);
    test_bc_queue_p(tc, bc, 30, 7, 1);
    delete_broken_collection(bc);

}

static const chunit_test BC_QUEUE_FRONT_DEL = {
    .name = "Broken Collection Queue Back with Deletion",
    .t = test_bc_queue_front_del,
    .timeout = 5,
};

static void test_bc_one(chunit_test_context *tc) {
    util_bc *bc = new_broken_collection(1, sizeof(uint64_t), 1, 1);
    test_bc_push_and_pop(tc, bc, 30, 1);
    test_bc_queue_p(tc, bc, 30, 7, 1);
    delete_broken_collection(bc);

    bc = new_broken_collection(1, sizeof(uint64_t), 1, 0);
    test_bc_push_and_pop(tc, bc, 25, 0);
    test_bc_queue_p(tc, bc, 30, 5, 1);
    delete_broken_collection(bc);
}

// Test a broken collection with table size 1.
static const chunit_test BC_ONE = {
    .name = "Broken Collection Table Size 1",
    .t = test_bc_one,
    .timeout = 5,
};

static void test_bc_pop_fail(chunit_test_context *tc) {
    util_bc *bc = new_broken_collection(1, sizeof(uint64_t), 1, 1);
    uint64_t i;
    bc_pop_front(bc, &i); // Should error.
    delete_broken_collection(bc);
}

static const chunit_test BC_POP_FAIL = {
    .name = "Broken Collection Pop Fail",
    .t = test_bc_pop_fail,
    .timeout = 5,
    .should_fail = 1,
};

static void test_bc_string(chunit_test_context *tc) {
    const uint64_t num_strings = 4;
    const uint64_t num_chars = 5;

    util_bc *bc = new_broken_collection(1, 
            sizeof(char) * num_chars, 1, 1);


    const char strs[num_strings][num_chars] = {
        "more",
        "yoyo",
        "AyYo",
        "Hexa",
    };

    uint64_t i;
    for (i = 0; i < num_strings; i++) {
        bc_push_back(bc, strs[i]);
    }
    
    char buf[num_chars];
    for (i = 0; i < num_strings; i++) {
        bc_pop_back(bc, buf);

        assert_eq_str(tc, strs[num_strings - 1 - i], buf);
    }

    delete_broken_collection(bc);
}

static const chunit_test BC_STRING = {
    .name = "Broken Collection String",
    .t = test_bc_string,
    .timeout = 5,
};

const chunit_test_suite UTIL_TEST_SUITE_BC = {
    .name = "Broken Collection Suite",
    .tests = {
        &BC_NEW,
        &BC_BACK,
        &BC_BACK_DEL,
        &BC_FRONT,
        &BC_FRONT_DEL,

        &BC_QUEUE_BACK,
        &BC_QUEUE_BACK_DEL,
        &BC_QUEUE_FRONT,
        &BC_QUEUE_FRONT_DEL,
        &BC_ONE,

        &BC_POP_FAIL,
        &BC_STRING,
    },
    .tests_len = 12
};

