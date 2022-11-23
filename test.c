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
#include "./gc_src/mem.h"

// This works!!!
#include <pthread.h>


int main(void) {
    slist *sl = new_slist(0, sizeof(int) * 2);

    int tups[4][2] = {
        {1, 2},
        {3, -6},
        {2, 3},
        {5, 1}
    };

    uint8_t len = 4;
    uint8_t i;
    for (i = 0; i < len; i++) {
        sl_add(sl, tups[i]);
    }

    sl_add(sl, tups[2]);
    sl_add(sl, tups[2]);
    sl_add(sl, tups[2]);
    sl_add(sl, tups[3]);
    sl_add(sl, tups[3]);
    sl_add(sl, tups[3]);
    sl_remove(sl, 3);

    for (i = 0; i < sl->len; i++) {
        int *cell = sl_get(sl, i);
        printf("TUP %d : [%d %d]\n", i, cell[0], cell[1]);
    }


    delete_slist(sl);

    // chunit_test_module_run *tmr = chunit_run_module_pb(&UTIL_TEST_MOD);
    // chunit_print_test_module_run(tmr);
    // chunit_delete_test_module_run(tmr);

    return 0;
}

