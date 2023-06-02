#include "./mod.h"

#include "./data.h"
#include "thread.h"

const chunit_test_module UTIL_TEST_MOD = {
    .name = "Utility Module",
    .suites = {
        &UTIL_TEST_SUITE_LL,
        &UTIL_TEST_SUITE_AR,
        &UTIL_TEST_SUITE_BC,
        &UTIL_TEST_SUITE_THREAD,
    },
    .suites_len = 3
};
