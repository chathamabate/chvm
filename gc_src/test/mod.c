#include "./mod.h"
#include "./virtual.h"

const chunit_test_module GC_TEST_MOD = {
    .name = "Garbage Collection Module",
    .suites = {
        &GC_TEST_SUITE_ADT,
    },
    .suites_len = 1
};
