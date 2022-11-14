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

// This works!!!
//

void test_dummy(int pipe_fd) {
    assert_eq_str(pipe_fd, "Hello", "World");
}

void test_dummy2(int pipe_fd) {
    exit(3);
}

const chunit_test TEST = {
    .name = "Test 1",
    .timeout = 5,
    .t = test_dummy,
};

const chunit_test TEST2 = {
    .name = "Test 2",
    .timeout = 5,
    .t = test_dummy2,
};

const chunit_test_suite SUITE = {
    .name = "Suite 1",
    .tests = {
        &TEST,
        &TEST,
        &TEST2
    },
    .tests_len = 3,
};

int main(void) {
    chunit_test_suite_run *tsr = chunit_run_suite(&SUITE);
    chunit_print_test_suite_run(tsr);
    chunit_delete_test_suite_run(tsr);
    return 0;
}
