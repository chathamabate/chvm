#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "testing_src/chunit.h"
#include "testing_src/assert.h"
#include "core_src/mem.h"
#include "core_src/data.h"
#include "core_src/log.h"
#include "testing_src/output.h"

#include "./util_src/data.h"

#include "./util_src/test/mod.h"

// This works!!!

int main(void) {
    // chunit_test_module_run *tmr = chunit_run_module_pb(&UTIL_TEST_MOD);
    chunit_test_run *tr = chunit_run_test(UTIL_TEST_MOD.suites[0]->tests[0], NULL, NULL);
    chunit_print_test_run(tr);
    chunit_delete_test_run(tr);
    /*
    chunit_print_test_module_run(tmr);
    chunit_delete_test_module_run(tmr);
    */
    return 0;
}

