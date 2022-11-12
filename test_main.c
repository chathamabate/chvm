#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include "testing_src/chunit.h"
#include "testing_src/assert.h"
#include "core_src/mem.h"
#include "core_src/data.h"
#include "core_src/log.h"
#include "testing_src/output.h"

// This works!!!
//

void test_dummy(int pipe_fd) {
    int x = 10;
    int b = 32;
    assert_eq_ptr(pipe_fd, &x, &x);
}

const chunit_test TEST = {
    .name = "Test 1",
    .timeout = 5,
    .t = test_dummy,
};

int main(void) {
    chunit_test_run *tr = chunit_run_test(&TEST);

    chunit_print_test_run(tr);

    chunit_delete_test_run(tr);
    return 0;
}
