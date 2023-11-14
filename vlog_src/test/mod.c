#include "./mod.h"

#include "./calc.h"

const chunit_test_module VLOG_TEST_MOD = {
    .name = "Calculator Module",
    .suites = {
        &ADD_SUITE,
        &MULT_SUITE
    },
    .suites_len = 2,
};
