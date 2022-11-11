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
    while(1) {} // SHUH YEH
}

const chunit_test TEST = {
    .name = "Test 1",
    .timeout = 5,
    .t = test_dummy,
};

int main(void) {
    chunit_test_run *tr = run_test(&TEST);

    
    print_test_run(tr);

    delete_test_run(tr);
    display_channels();

    return 1;
}
