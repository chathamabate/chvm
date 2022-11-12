
#include "./output.h"
#include "chunit.h"
#include <stdio.h>
#include "../core_src/log.h"
#include <inttypes.h>

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
    PRINT_RESULT(prefix, s_style, n_style, plh, "Acutal   : ", act)

static void print_tr_success(const char *prefix, chunit_test_run *tr) {
    printf("%s" S_SUCC UC_HORIZ_LINE UC_HORIZ_LINE " %s" CC_RESET 
            N_SUCC " (Success)" CC_RESET "\n", 
            prefix, tr->test->name);
}

static void print_tr_warn(const char *prefix, chunit_test_run *tr) {
    printf("%s" S_WARN UC_TL_CORNER UC_HORIZ_LINE " %s" CC_RESET 
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
            break;

        case CHUNIT_TIMEOUT:
            break;

        default:
            break;
    }

    printf(S_WARN UC_BL_CORNER UC_HORIZ_LINE CC_RESET "\n");
}

static void print_tr_fail(const char *prefix, chunit_test_run *tr) {
    printf("%s" S_FAIL UC_HORIZ_LINE UC_HORIZ_LINE " %s" CC_RESET 
            N_FAIL " (Failure)" CC_RESET "\n", 
            prefix, tr->test->name);
}

static void print_tr_framework_err(const char *prefix, chunit_test_run *tr) {
    printf("%s" S_FRER UC_TL_CORNER UC_HORIZ_LINE " %s" CC_RESET 
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
    
    printf(S_FRER UC_BL_CORNER UC_HORIZ_LINE CC_RESET "\n");

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

