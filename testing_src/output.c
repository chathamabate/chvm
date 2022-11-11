
#include "./output.h"
#include "chunit.h"
#include <stdio.h>
#include "../core_src/log.h"

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


// N for normal.
// S for strong.

#define N_SUCC CC_BRIGHT_GREEN
#define S_SUCC CC_BRIGHT_GREEN CC_BOLD

#define N_WARN CC_BRIGHT_YELLOW
#define S_WARN CC_BRIGHT_YELLOW CC_BOLD

#define N_FAIL CC_BRIGHT_RED
#define S_FAIL CC_BRIGHT_RED CC_BOLD

// FRER = Framework Error.
#define N_FRER CC_BRIGHT_MAGENTA
#define S_FRER CC_BRIGHT_MAGENTA CC_BOLD

static void print_tr_success(const char *prefix, chunit_test_run *tr) {

}

static void print_tr_warn(const char *prefix, chunit_test_run *tr) {

}

static void print_tr_fail(const char *prefix, chunit_test_run *tr) {

}

static void print_tr_framework_err(const char *prefix, chunit_test_run *tr) {

}

static void print_test_run(const char *prefix, chunit_test_run *tr) {
    if (tr->errors->len > 0 || tr->result == CHUNIT_VOID) {
        print_tr_framework_err(prefix, tr);
    } else if (tr->result == CHUNIT_FATAL_ERROR) {
        print_tr_fail(prefix, tr);
    } else if (tr->result == CHUNIT_SUCCESS) {
        print_tr_success(prefix, tr);
    } else {
        print_tr_warn(prefix, tr);
    }
}

void chunit_print_test_run(chunit_test_run *tr) {
    print_test_run("", tr);
}

