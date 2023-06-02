#include "core_src/io.h"
#include "testing_src/chunit.h"
#include "testing_src/output.h"

#include "core_src/sys.h"

#include "./util_src//test/mod.h"
#include "./gc_src//test/mod.h"

#include "./util_src/data.h"

#define MODULES_LEN 2

static const chunit_test_module *MODULES[MODULES_LEN] = {
    &UTIL_TEST_MOD,
    &GC_TEST_MOD,
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

static void vlog_code(void);

int main(void) {
    init_core_state(8);

    // vlog_code();    

    // safe_exit(0);
    int c = safe_main();

    // NOTE this is needed.
    safe_exit(c);

    // Should never make it here.
    return 1;
    
}







#include "./gc_src/virt.h"  // Virtual Memory.
#include "./gc_src/mb.h"    // Memory Block. 
#include "./gc_src/ms.h"    // Memory Space.
#include "./gc_src/cs.h"    // Collected Space.

static void bubblesort(int *int_arr, int int_arr_len) {

}

static void vlog_code(void) {
    
}

