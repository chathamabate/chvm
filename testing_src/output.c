
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

static void print_test_run(const char *prefix, chunit_test_run *tr) {
}

void chunit_print_test_run(chunit_test_run *tr) {
    print_test_run("", tr);
}

