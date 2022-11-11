
#include "./output.h"
#include "chunit.h"
#include <stdio.h>

const char *CHUNIT_TR_NAMES[CHUNIT_FATAL_ERROR + 1] = {
    "Void",
    "Success",
    "Assert True Failure",
    "Assert False Failure",
    "Assert Non-Null Failure",
    "Assert Equal Pointer Failure",
    "Assert Equal Integer Failure",
    "Assert Equal Unsigned Integer Failure",
    "Assert Equal Character Failure",
    "Assert Equal String Failure",
    "Memory Leak",
    "Timeout",
    "Fatal Error", 
};

const char *CHUNIT_FE_NAMES[CHUNIT_TERMINATION_ERROR + 1] = {
    "Pipe Error",
    "Fork Error",
    "Bad Test Result",
    "Termination Error",
};

void print_test_run(chunit_test_run *tr) {
    printf("PID : %d\n", tr->child);
    printf("RES : %s\n", CHUNIT_TR_NAMES[tr->result]);
    printf("ERR : \n");

    uint64_t err_i;
    for (err_i = 0; err_i < tr->errors->len; err_i++) {
        chunit_framework_error err = 
            ((chunit_framework_error *)(tr->errors->buf))[err_i];

        printf("    : %s\n", CHUNIT_FE_NAMES[err]);
    }
}
