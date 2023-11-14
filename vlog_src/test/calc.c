
#include "./calc.h"
#include "../../testing_src/assert.h"

#include "../calc.h"

static void test_vc_add(chunit_test_context *tc) {
    int64_t x = 10, y = 15; 
    int64_t exp_result = x + y;
    int64_t actual_result = vc_add(x, y);

    assert_eq_int(tc, exp_result, actual_result);
}

static const chunit_test VC_ADD = {
    .name = "Calculator Add",
    .timeout = 5,
    .t = test_vc_add,
};


const chunit_test_suite ADD_SUITE = {
    .name = "Addition Suite",
    .tests = {
        &VC_ADD,
        &VC_ADD,
        &VC_ADD,
        &VC_ADD,
    },
    .tests_len = 4,
};

static void test_vc_mult1(chunit_test_context *tc) {
    assert_eq_int(tc, -3, 3);
}

static const chunit_test VC_MULT1 = {
    .name = "Negative * Positive",
    .timeout = 5,
    .t = test_vc_mult1
};

static void test_vc_mult2(chunit_test_context *tc) {
    int x = 0, y = 1;
    y /= x;
    (void)y;
}

static const chunit_test VC_MULT2 = {
    .name = "Negative * Negative",
    .timeout = 5,
    .t = test_vc_mult2
};

const chunit_test_suite MULT_SUITE = {
    .name = "Multiplication Suite",
    .tests = {
        &VC_MULT1,
        &VC_MULT2,
        &VC_ADD,
        &VC_ADD,
        &VC_ADD,
    },
    .tests_len = 5,
};
