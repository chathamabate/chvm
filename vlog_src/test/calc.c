
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


const chunit_test_suite CALC_SUITE = {
    .name = "Calculator Suite",
    .tests = {
        &VC_ADD,
    },
    .tests_len = 1,
};
