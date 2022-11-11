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
    // FLAMES
    printf(CC_GREEN  " " UC_TL_CORNER UC_HORIZ_LINE " Hello 1\n");
    printf(CC_BRIGHT_GREEN UC_SHIP_WHEEL UC_VERTICAL_LINE  "\n");
    printf(CC_BRIGHT_GREEN UC_SHIP_WHEEL UC_VERTICAL_LINE  "\n");
    printf(CC_BRIGHT_GREEN UC_SHIP_WHEEL UC_VERTICAL_LINE  "\n");
    printf(CC_BRIGHT_GREEN UC_SHIP_WHEEL UC_VERTICAL_LINE  "\n");

    return 1;
}
