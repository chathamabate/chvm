#include "./mod.h"

#include "./calc.h"

const chunit_test_module VLOG_TEST_MOD = {
    .name = "Vlog Module",
    .suites = {
        &CALC_SUITE,
    },
    .suites_len = 1,
};
