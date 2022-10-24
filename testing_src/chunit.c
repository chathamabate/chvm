#include "chunit.h"

#include "../core_src/mem.h"

// All testing work should be done in here...
// This will be called in the parent process.
chunit_result *chunit_run_test(const chunit_test *test) {

    return NULL;
}

void delete_chunit_result(chunit_result *res) {
    if (!res) {
        return;
    }

    if (res->s == CHUNIT_ASSERTION_ERROR) {
        if (res->assertion_failure->assert_tag == CHUNIT_ASSERT_EQ_STR) {
            chunit_str_cmpr *str_cmpr = &(res->assertion_failure->cmpr.str_cmpr);
            
            // Free hanging strings.
            safe_free(MEM_CHNL_TESTING, str_cmpr->expected);
            safe_free(MEM_CHNL_TESTING, str_cmpr->actual);
        }
        
        // Free assertion failure itself.
        safe_free(MEM_CHNL_TESTING, res->assertion_failure);
    }

    // Finally free the result.
    safe_free(MEM_CHNL_TESTING, res);
}

// Below calls will be called from the child process.
void assert_true(int pipe_fd, int actual) {

}

void assert_false(int pipe_fd, int actual) {

}

void assert_non_null(int pipe_fd, void *ptr) {

}

void assert_eq_ptr(int pipe_fd, void *expected, void *actual) {

}

void assert_eq_int(int pipe_fd, int64_t expected, int64_t actual) {

}

void assert_eq_uint(int pipe_fd, uint64_t expected, uint64_t actual) {

}

void assert_eq_char(int pipe_fd, char expected, char actual) {

}

void assert_eq_str(int pipe_fd, char *expected, char *actual) {

}
