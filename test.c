#include "core_src/io.h"
#include "testing_src/chunit.h"
#include "testing_src/output.h"

#include "core_src/sys.h"

#include "./util_src/test/mod.h"
#include "./gc_src/test/mod.h"
#include "./vlog_src/test/mod.h"

#include "./util_src/data.h"

#include "./vlog_src/main.h"


#define MODULES_LEN 3

static const chunit_test_module *MODULES[MODULES_LEN] = {
    &UTIL_TEST_MOD,
    &GC_TEST_MOD,
    &VLOG_TEST_MOD,
};

static int safe_main(void) {
    chunit_test_module_run *tmrs[MODULES_LEN];

    uint64_t i;
    for (i = 0; i < MODULES_LEN; i++) {
        tmrs[i] = chunit_run_module_pb(MODULES[i]);
    }

    safe_printf("\n");

    for (i = 0; i < MODULES_LEN; i++) {
        chunit_print_test_module_run(tmrs[i]);
        chunit_delete_test_module_run(tmrs[i]);
    }

    return 0;
}

int main(void) {
    init_core_state(8);

    // int c = safe_main();
    int c = vlog_main();

    // NOTE this is needed.
    safe_exit(c);

    // I like to use this part a lot.

    // Should never make it here.
    return 1;
}

