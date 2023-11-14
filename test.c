#include "core_src/io.h"
#include "gc_src/virt.h"
#include "testing_src/chunit.h"
#include "testing_src/output.h"

#include "core_src/sys.h"

#include "./util_src/test/mod.h"
#include "./gc_src/test/mod.h"
#include "./vlog_src/test/mod.h"

#include "./util_src/data.h"

#include "gc_src/cs.h"

#include "./vlog_src/main.h"
#include <time.h>


#define MODULES_LEN 1

static const chunit_test_module *MODULES[MODULES_LEN] = {
    // &UTIL_TEST_MOD,
    // &GC_TEST_MOD,
    &VLOG_TEST_MOD,
};

static int safe_main(void) {
    collected_space *cs = new_collected_space(1, 5, 1000);

    // Create a single root object.
    addr_book_vaddr rv = cs_malloc_object(cs, 5, 0);
    cs_root_id rid = cs_root(cs, rv);

    // Collection cycle every 5 seconds.
    const struct timespec TSPEC = {
        .tv_sec = 5
    };

    // Arguments for GC algorithm.
    const gc_worker_spec SPEC = {
        .delay = &TSPEC,
        .shift = 1, .shift_trigger = 50
    };

    // Start up GC on a background thread.
    cs_start_gc(cs, &SPEC);

    // Safely access our root object in write mode.
    obj_header *root_h = cs_get_write(cs, rv);


    (void)rid;
    (void)root_h;

    delete_collected_space(cs);

    return 0;

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

    int c = safe_main();
    //int c = vlog_main();

    // NOTE this is needed.
    safe_exit(c);

    // I like to use this part a lot.

    // Should never make it here.
    return 1;
}

