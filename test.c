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
#include "core_src/sys.h"

#include "./util_src/test/mod.h"
#include "./gc_src/mem.h"

// This works!!!
#include <pthread.h>


int main(void) {
    core_log("HELLO \n");
    core_logf("This number is important (%d)\n", -456);
    // chunit_test_module_run *tmr = chunit_run_module_pb(&UTIL_TEST_MOD);
    // chunit_print_test_module_run(tmr);
    // chunit_delete_test_module_run(tmr);

    return 0;
}

