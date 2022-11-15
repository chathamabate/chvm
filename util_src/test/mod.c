#include "./mod.h"

#include "./data.h"

const chunit_test_module UTIL_TEST_MOD = {
    .name = "Utility Module",
    .suites = {
        &UTIL_TEST_SUITE_LL
    },
    .suites_len = 1
};
