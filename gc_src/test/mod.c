#include "./mod.h"
#include "./virt.h"
#include "./mb.h"
#include "cs.h"
#include "ms.h"

const chunit_test_module GC_TEST_MOD = {
    .name = "Garbage Collection Module",
    .suites = {
        &GC_TEST_SUITE_ADT,
        &GC_TEST_SUITE_ADB,
        &GC_TEST_SUITE_MB,
        &GC_TEST_SUITE_MS,
        &GC_TEST_SUITE_CS,
    },
    .suites_len = 5
};
