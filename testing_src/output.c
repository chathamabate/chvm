
#include "./output.h"
#include "./chunit.h"
#include <stdio.h>
#include "../core_src/log.h"
#include <inttypes.h>
#include "../core_src/mem.h"

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

#define N_SUCC CC_GREEN
#define S_SUCC CC_BRIGHT_GREEN CC_BOLD

#define N_WARN CC_YELLOW
#define S_WARN CC_BRIGHT_YELLOW CC_BOLD

#define N_FAIL CC_RED
#define S_FAIL CC_BRIGHT_RED CC_BOLD

// FRER = Framework Error.
#define N_FRER CC_MAGENTA
#define S_FRER CC_BRIGHT_MAGENTA CC_BOLD

#define BULLET_PREFIX UC_VERTICAL_LINE " " UC_SHIP_WHEEL " "

#define PRINT_RESULT(prefix, s_style, n_style, plh, name, res) \
    printf("%s" s_style BULLET_PREFIX CC_RESET n_style name plh CC_RESET "\n", \
            prefix, res)

#define PRINT_COMPARISON(prefix, s_style, n_style, plh, exp, act) \
    PRINT_RESULT(prefix, s_style, n_style, plh, "Expected : ", exp); \
    PRINT_RESULT(prefix, s_style, n_style, plh, "Actual   : ", act)

#define TL_CORNER_FMT(s) s UC_TL_CORNER UC_HORIZ_LINE " %s" CC_RESET
#define HEADER_FMT(s) s UC_HORIZ_LINE UC_HORIZ_LINE " %s" CC_RESET
#define SEG_FMT(s) s UC_HORIZ_LINE UC_HORIZ_LINE CC_RESET
#define BL_CORNER_FMT(s) s UC_BL_CORNER UC_HORIZ_LINE CC_RESET


static void print_tr_success(const char *prefix, chunit_test_run *tr) {
    printf("%s" HEADER_FMT(S_SUCC) 
            N_SUCC " (Success)" CC_RESET "\n", 
            prefix, tr->test->name);
}

static void print_tr_warn(const char *prefix, chunit_test_run *tr) {
    printf("%s" TL_CORNER_FMT(S_WARN) 
            N_WARN " (Warning)" CC_RESET "\n", 
            prefix, tr->test->name);

    printf("%s" S_WARN UC_VERTICAL_LINE " %s" CC_RESET "\n", 
            prefix, CHUNIT_TR_NAMES[tr->result]);

    switch (tr->result) {
        case CHUNIT_ASSERT_EQ_PTR_FAIL:
            PRINT_COMPARISON(prefix, S_WARN, N_WARN, "%p", 
                    ((void **)(tr->data))[0], ((void **)(tr->data))[1]);
            break;

        case CHUNIT_ASSERT_EQ_INT_FAIL:
            PRINT_COMPARISON(prefix, S_WARN, N_WARN, "%" PRId64, 
                    ((int64_t *)(tr->data))[0], ((int64_t *)(tr->data))[1]);

            break;

        case CHUNIT_ASSERT_EQ_UINT_FAIL:
            PRINT_COMPARISON(prefix, S_WARN, N_WARN, "%" PRIu64, 
                    ((uint64_t *)(tr->data))[0], ((uint64_t *)(tr->data))[1]);
            break;

        case CHUNIT_ASSERT_EQ_CHAR_FAIL:
            PRINT_COMPARISON(prefix, S_WARN, N_WARN, "0x%02X", 
                    ((char *)(tr->data))[0], ((char *)(tr->data))[1]);

            break;

        case CHUNIT_ASSERT_EQ_STR_FAIL:
            PRINT_COMPARISON(prefix, S_WARN, N_WARN, "%s", 
                    ((char **)(tr->data))[0], ((char **)(tr->data))[1]);

            break;

        case CHUNIT_MEMORY_LEAK:
            // Nothing really to do here.
            break;

        case CHUNIT_TIMEOUT:
            printf("%s" S_WARN BULLET_PREFIX CC_RESET N_WARN
                    "Process exceeded specified time limit (%lu)" CC_RESET "\n", 
                    prefix, tr->test->timeout);
            break;

        default:
            // Other types are not warnings.
            // Should never make it here.
            break;
    }

    printf("%s" BL_CORNER_FMT(S_WARN) "\n", prefix);
}

static void print_tr_fail(const char *prefix, chunit_test_run *tr) {
    printf("%s" HEADER_FMT(S_FAIL) 
            N_FAIL " (Failure)" CC_RESET "\n", 
            prefix, tr->test->name);
}

static void print_tr_framework_err(const char *prefix, chunit_test_run *tr) {
    printf("%s" TL_CORNER_FMT(S_FRER)
            N_FRER " (Framework Error)" CC_RESET "\n", 
            prefix, tr->test->name);

    uint64_t i;
    for (i = 0; i < tr->errors->len; i++) {
        chunit_framework_error fr_err = 
            *(chunit_framework_error *)sl_get(tr->errors, i);

        const char *err_msg = CHUNIT_FE_NAMES[fr_err];

        printf("%s" S_FRER UC_VERTICAL_LINE " %s" CC_RESET "\n", 
                prefix, err_msg);

        if (fr_err == CHUNIT_TERMINATION_ERROR) {
            printf("%s" S_FRER BULLET_PREFIX CC_RESET 
                    N_FRER  "Consider confirming process %d has terminated." CC_RESET "\n", 
                    prefix, tr->child);
        }
    }
    
    printf("%s" BL_CORNER_FMT(S_FRER) "\n", prefix);

}

static void print_test_run(const char *prefix, chunit_test_run *tr) {
    if (tr->errors->len > 0 || tr->result == CHUNIT_VOID) {
        print_tr_framework_err(prefix, tr);
    } else if (tr->result == CHUNIT_FATAL_ERROR) {
        print_tr_fail(prefix, tr);
    } else if (tr->result == CHUNIT_SUCCESS) {
        // This is actually fine with me.
        // We will only output as a success if and only if
        // The result is success and there were no framework errors.
        print_tr_success(prefix, tr);
    } else {
        print_tr_warn(prefix, tr);
    }
}

void chunit_print_test_run(chunit_test_run *tr) {
    print_test_run("", tr);
}

static void print_test_suite_run(const char *prefix, chunit_test_suite_run *tsr) {
    if (tsr->successes == tsr->suite->tests_len) {
        printf("%s" HEADER_FMT(S_SUCC) 
                N_SUCC " (%" PRIu64 " of %" PRIu64 " Tests Succeeded)" CC_RESET "\n", 
                prefix, tsr->suite->name, tsr->successes, tsr->suite->tests_len);
    } else {
        printf("%s" TL_CORNER_FMT(S_FAIL) 
                N_FAIL " (%" PRIu64  " of %" PRIu64 " Tests Failed)" CC_RESET "\n",
                prefix, tsr->suite->name, tsr->suite->tests_len - tsr->successes, tsr->suite->tests_len);

        // Here we must make our new prefix!
        char *new_prefix = 
            concat_str(MEM_CHNL_TESTING, prefix, S_FAIL UC_VERTICAL_LINE " " CC_RESET);

        uint64_t i;
        for (i = 0; i < tsr->test_runs->len; i++) {
            chunit_test_run *tr = *(chunit_test_run **)sl_get(tsr->test_runs, i);

            // If we find an unsuccessful test, print it out.
            if (tr->result != CHUNIT_SUCCESS || tr->errors->len > 0) {
                print_test_run(new_prefix, tr);
            }
        }

        safe_free(new_prefix);

        printf("%s" BL_CORNER_FMT(S_FAIL) "\n", prefix);
    }
}

// A suite run will print all failed tests.
// If there were no failed tests, just a single line saying
// the name of the suite and how many tests passed.
//
// How should it say the tests passed?
// How about if they failed?
//
// 100 / 100 

void chunit_print_test_suite_run(chunit_test_suite_run *tsr) {
    print_test_suite_run("", tsr);
}

#define S_MOD

// Don't really need to give a prefix for this.
void chunit_print_test_module_run(chunit_test_module_run *tmr) {
    if (tmr->mod->suites_len == 0) {
        printf(HEADER_FMT(S_MOD) "\n", tmr->mod->name);
    } else {
        printf(TL_CORNER_FMT(S_MOD) "\n", tmr->mod->name);
        
        uint64_t i;
        for (i = 0; i < tmr->test_suite_runs->len; i++) {
            chunit_test_suite_run *tsr = 
                *(chunit_test_suite_run **)sl_get(tmr->test_suite_runs, i);

            print_test_suite_run(S_MOD UC_VERTICAL_LINE " " CC_RESET, tsr);
        }

        printf(BL_CORNER_FMT(S_MOD) "\n");
    }
}


