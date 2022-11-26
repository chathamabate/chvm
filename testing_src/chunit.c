#include "./chunit.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "../core_src/mem.h"
#include "../core_src/log.h"
#include "../core_src/sys.h"
#include "../core_src/io.h"
#include "./assert.h"


// Parent process and Child process code for CHUNIT.

static chunit_test_run *new_test_run(const chunit_test *test, pid_t c) {
    chunit_test_run *tr = safe_malloc(MEM_CHNL_TESTING, sizeof(chunit_test_run));
    tr->errors = new_slist(MEM_CHNL_TESTING, sizeof(chunit_framework_error));

    tr->test = test;
    tr->child = c;
    tr->result = CHUNIT_VOID;
    tr->data = NULL;

    return tr;
}

void chunit_delete_test_run(chunit_test_run *tr) {
    switch (tr->result) {
        // Simple comparisons.
        case CHUNIT_ASSERT_EQ_PTR_FAIL:
        case CHUNIT_ASSERT_EQ_INT_FAIL:
        case CHUNIT_ASSERT_EQ_UINT_FAIL:
        case CHUNIT_ASSERT_EQ_CHAR_FAIL:
            safe_free(tr->data);
            break;

        case CHUNIT_ASSERT_EQ_STR_FAIL:
            safe_free(((char **)(tr->data))[0]); // Free Expected.
            safe_free(((char **)(tr->data))[1]); // Free Actual.
            safe_free(tr->data); // Free buffer.
            break;

        default:
            // Otherwise, nothing should be in data.
            break;
    }

    delete_slist(tr->errors);
    safe_free(tr);
}

// NOTE fds[1] = writing descriptor.
// fds[0] = reading descriptor.

static void add_framework_error(chunit_test_run *tr, chunit_framework_error err) {
    sl_add(tr->errors, &err);
}

static void attempt_safe_kill_and_reap(chunit_test_run *tr) {
    if (safe_kill_and_reap(tr->child)) {
        add_framework_error(tr, CHUNIT_TERMINATION_ERROR);
    }
}

static void attempt_safe_close(chunit_test_run *tr, int pipe_fd) {
    if (safe_close(pipe_fd)) {
        add_framework_error(tr, CHUNIT_PIPE_ERROR);
    }
}

static void attempt_read_cmpr_and_close(chunit_test_run *tr, int pipe_fd, 
        chunit_test_result res, size_t c_size) {
    void *buf = safe_malloc(MEM_CHNL_TESTING, c_size * 2);

    if (safe_read(pipe_fd, buf, c_size * 2)) {
        add_framework_error(tr, CHUNIT_PIPE_ERROR);
        safe_free(buf);

        // No closing here.
        return;
    }

    // Successful read!
    tr->data = buf;
    tr->result = res;

    // Do this here instead of later.
    attempt_safe_close(tr, pipe_fd);
}

static chunit_test_run *chunit_parent_process(int fds[2], const chunit_test *test, pid_t child) {

    // Test run to return.
    chunit_test_run *tr = new_test_run(test, child);

    // NOTE : going forward in the parent process... 
    // If there is a non-pipe error, we must close the read end of
    // the pipe before exiting!

    // Attempt to close write end of pipe.
    if (safe_close(fds[1])) {
        add_framework_error(tr, CHUNIT_PIPE_ERROR);
        attempt_safe_kill_and_reap(tr);

        // Since this is a pipe error, don't try to close
        // read end of the pipe.

        return tr;
    }

    // Get read end of the pipe
    int pipe_fd = fds[0];
    int stat;
    
    // Wait for our boy to finish up here!
    pid_t res = safe_waitpid_timed(child, &stat, test->timeout); 

    // When there's a hard waitpid error, just return...
    // don't worry about killing.
    if (res == -1) {
        add_framework_error(tr, CHUNIT_TERMINATION_ERROR);
        attempt_safe_close(tr, pipe_fd);

        return tr;
    }

    // There's been a timeout, kill, reap, close, and return.
    if (res == -2) {
        tr->result = CHUNIT_TIMEOUT;
        attempt_safe_kill_and_reap(tr);
        attempt_safe_close(tr, pipe_fd);

        return tr;
    }

    // Here the program terminated on its own...
    // check to see if it errored out.

    // If the child didn't exit normally, return a fatal
    // error. (User's fault)
    if (!WIFEXITED(stat)) {
        tr->result = CHUNIT_FATAL_ERROR;
        attempt_safe_close(tr, pipe_fd);

        return tr;
    }

    // If we make it here, we must've exited normally.

    // Check if there was some pipe error. (OUR ERROR)
    if (WEXITSTATUS(stat) == CHUNIT_PIPE_ERROR_EXIT_CODE) {
        add_framework_error(tr, CHUNIT_PIPE_ERROR);
        // pipe doesn't need to be closed.
        
        return tr;
    }

    // This is a positive exit status outside of the pipe error exit code.
    if (WEXITSTATUS(stat) > 0) {
        // Ashamed of the copy and paste which occured here.
        tr->result = CHUNIT_FATAL_ERROR;
        attempt_safe_close(tr, pipe_fd);

        return tr;
    }

    // This means the process terminated on it's own
    // and it wasn't some fatal error/pipe error.
    // Now we must interpret the results.
    
    chunit_test_result t_res;
    
    // Read comm tag.
    if (safe_read(pipe_fd, &t_res, sizeof(chunit_test_result))) {
        add_framework_error(tr, CHUNIT_PIPE_ERROR);

        return tr;
    }

    // NOTE, consider creating a results interpretation helper function.

    size_t str_sizes[2];

    switch (t_res) {
        // No data field needed in below cases.
        case CHUNIT_SUCCESS:
        case CHUNIT_MEMORY_LEAK:
        case CHUNIT_ASSERT_TRUE_FAIL:
        case CHUNIT_ASSERT_FALSE_FAIL:
        case CHUNIT_ASSERT_NON_NULL_FAIL:
            tr->result = t_res;
            attempt_safe_close(tr, pipe_fd);

            return tr;

        case CHUNIT_ASSERT_EQ_PTR_FAIL:
            attempt_read_cmpr_and_close(tr, pipe_fd, 
                    CHUNIT_ASSERT_EQ_PTR_FAIL, sizeof(void *));

            return tr;

        case CHUNIT_ASSERT_EQ_INT_FAIL:
            attempt_read_cmpr_and_close(tr, pipe_fd, 
                    CHUNIT_ASSERT_EQ_INT_FAIL, sizeof(int64_t));

            return tr;

        case CHUNIT_ASSERT_EQ_UINT_FAIL:
            attempt_read_cmpr_and_close(tr, pipe_fd, 
                    CHUNIT_ASSERT_EQ_UINT_FAIL, sizeof(uint64_t));

            return tr;

        case CHUNIT_ASSERT_EQ_CHAR_FAIL:
            attempt_read_cmpr_and_close(tr, pipe_fd, 
                    CHUNIT_ASSERT_EQ_CHAR_FAIL, sizeof(char));

            return tr;

        case CHUNIT_ASSERT_EQ_STR_FAIL:
            // Read sizes first.
            if (safe_read(pipe_fd, str_sizes, sizeof(size_t) * 2)) {
                add_framework_error(tr, CHUNIT_PIPE_ERROR);

                return tr;
            }

            char *expected = safe_malloc(MEM_CHNL_TESTING, str_sizes[0]);

            // Read expected.
            if (safe_read(pipe_fd, expected, str_sizes[0])) {
                add_framework_error(tr, CHUNIT_PIPE_ERROR);

                safe_free(expected);
                return tr;
            }

            char *actual = safe_malloc(MEM_CHNL_TESTING, str_sizes[1]);

            // Read actual.
            if (safe_read(pipe_fd, actual, str_sizes[1])) {
                add_framework_error(tr, CHUNIT_PIPE_ERROR);

                safe_free(expected);
                safe_free(actual);
                return tr;
            }

            // We have successfully read both strings!
            tr->result = CHUNIT_ASSERT_EQ_STR_FAIL;
            tr->data = safe_malloc(MEM_CHNL_TESTING, sizeof(char *) * 2);

            // Copy in string pointers.
            ((char **)(tr->data))[0] = expected;
            ((char **)(tr->data))[1] = actual;

            attempt_safe_close(tr, pipe_fd);
            
            return tr; 

        default:
            add_framework_error(tr, CHUNIT_BAD_TEST_RESULT);
            attempt_safe_close(tr, pipe_fd);

            return tr;
    }
}

chunit_test_run *chunit_run_test(const chunit_test *test,
        const chunit_test_decorator *decorator, void *test_context) {
    int fds[2];

    if (pipe(fds)) {
        chunit_test_run *tr = new_test_run(test, -1);
        add_framework_error(tr, CHUNIT_PIPE_ERROR);
        return tr;
    }

    pid_t pid = fork();

    if (pid == -1) {
        chunit_test_run *tr = new_test_run(test, -1);
        add_framework_error(tr, CHUNIT_FORK_ERROR);
        return tr;
    }

    if (pid == 0) {
        // NOTE, we should not leave this call...
        // i.e. chunit_child_process must exit!
        chunit_child_process(fds, test);    
    }

    if (decorator) {
        decorator->start_test(test, pid, test_context);
    }

    chunit_test_run *tr = chunit_parent_process(fds, test, pid);
    
    if (decorator) {
        decorator->end_test(tr, test_context);
    }

    return tr;
}

static chunit_test_suite_run *new_test_suite_run(const chunit_test_suite *suite) {
    chunit_test_suite_run *tsr = 
        safe_malloc(MEM_CHNL_TESTING, sizeof(chunit_test_suite_run));

    tsr->suite = suite;
    tsr->successes = 0;
    tsr->test_runs = new_slist(MEM_CHNL_TESTING, sizeof(chunit_test_run *));

    return tsr;
}

chunit_test_suite_run *chunit_run_suite(const chunit_test_suite *suite, 
        const chunit_test_suite_decorator *decorator, void *suite_context) {
    chunit_test_suite_run *tsr = new_test_suite_run(suite);

    uint64_t i;
    for (i = 0; i < tsr->suite->tests_len; i++) {
        chunit_test_run *tr;
        if (decorator) {
            void *test_context = decorator->get_test_context(suite, i, suite_context);
            tr = chunit_run_test(suite->tests[i], 
                    decorator->test_decorator, test_context);
            decorator->cleanup_test_context(test_context);
        } else {
            tr = chunit_run_test(suite->tests[i], NULL, NULL);
        }

        sl_add(tsr->test_runs, &tr); // add test run to result list.

        if (tr->result == CHUNIT_SUCCESS && tr->errors->len == 0) {
            tsr->successes++;
        }
    }

    return tsr;
}

void chunit_delete_test_suite_run(chunit_test_suite_run *tsr) {
    // First we must delete all results. 
    uint64_t i;
    for (i = 0; i < tsr->test_runs->len; i++) {
        chunit_test_run *tr = *(chunit_test_run **)sl_get(tsr->test_runs, i);
        chunit_delete_test_run(tr);
    }

    // Then delete results list.
    delete_slist(tsr->test_runs);

    // Finally, delete tsr.
    safe_free(tsr);
}

static chunit_test_module_run *
new_test_module_run(const chunit_test_module *mod) {
    chunit_test_module_run *tmr = 
        safe_malloc(MEM_CHNL_TESTING, sizeof(chunit_test_module_run));

    tmr->mod = mod;
    tmr->successful = 1;
    tmr->test_suite_runs = 
        new_slist(MEM_CHNL_TESTING, sizeof(chunit_test_suite_run *));

    return tmr;
}

chunit_test_module_run *chunit_run_module(const chunit_test_module *mod,
        const chunit_test_module_decorator *decorator, void *mod_context) {
    chunit_test_module_run *tmr = new_test_module_run(mod);

    uint64_t i;
    for (i = 0; i < mod->suites_len; i++) {

        chunit_test_suite_run *tsr;
        if (decorator) {
            void *suite_context = decorator->get_suite_context(mod, i, mod_context);
            tsr = chunit_run_suite(mod->suites[i], 
                    decorator->suite_decorator, suite_context);
            decorator->cleanup_suite_context(suite_context);
        } else {
            tsr = chunit_run_suite(mod->suites[i], NULL, NULL);
        }

        sl_add(tmr->test_suite_runs, &tsr);

        if (tmr->successful && !(tsr->successes == tsr->suite->tests_len)) {
            tmr->successful = 0;
        }
    }

    return tmr;
}

void chunit_delete_test_module_run(chunit_test_module_run *tmr) {
    uint64_t i;
    for (i = 0; i < tmr->test_suite_runs->len; i++) {
        chunit_test_suite_run *tsr = 
            *(chunit_test_suite_run **)sl_get(tmr->test_suite_runs, i);
        chunit_delete_test_suite_run(tsr);
    }

    delete_slist(tmr->test_suite_runs);
    safe_free(tmr);
}

// Progress Bar Decorator.

typedef struct {
    const chunit_test_module *mod;
    uint64_t suite_i;

    uint64_t total_tests;   
    uint64_t completed_tests;
} chunit_pbar_context;

#define TESTING_CHUNIT_PBAR_WIDTH 20
#define TESTING_CHUNIT_PBAR_CHAR  '#'

static void chunit_fill_pbar(char pbar[], uint64_t amt, uint64_t tot) {
    uint16_t progress =  (uint16_t)(TESTING_CHUNIT_PBAR_WIDTH * 
            (amt / (double)(tot)));

    uint16_t i;
    for (i = 0; i < progress; i++) {
        pbar[i] = TESTING_CHUNIT_PBAR_CHAR;
    }

    for (; i < TESTING_CHUNIT_PBAR_WIDTH; i++) {
        pbar[i] = '_';
    }
    
    pbar[TESTING_CHUNIT_PBAR_WIDTH] = '\0';
}

#define TESTING_CHUNIT_PBAR_FMT "\33[2K\r" CC_BRIGHT_BLUE "%-20.20s"  \
    CC_BRIGHT_MAGENTA "%s" CC_RESET

static void chunit_pbar_start_test(const chunit_test *test, 
        pid_t pid, void *test_context) {

    chunit_pbar_context *pbar_c = (chunit_pbar_context *)test_context;
    char pbar[TESTING_CHUNIT_PBAR_WIDTH + 1];
    chunit_fill_pbar(pbar, pbar_c->completed_tests, pbar_c->total_tests);

    const chunit_test_module *mod = pbar_c->mod;
    const chunit_test_suite *suite = mod->suites[pbar_c->suite_i];

    // Module Name ( progress bar ) suite_name test_name
    printf(TESTING_CHUNIT_PBAR_FMT " %s" CC_BOLD CC_BRIGHT_MAGENTA " :: " CC_RESET
            CC_ITALIC CC_FAINT "%s [%d]" CC_RESET, pbar_c->mod->name, pbar, 
            pbar_c->mod->suites[pbar_c->suite_i]->name,
            test->name, pid);

    // Could do an error check... but ehh. not that important.
    fflush(stdout);
}

static void chunit_pbar_end_test(chunit_test_run *run, void *test_context) {
    chunit_pbar_context *pbar_c = (chunit_pbar_context *)test_context;
    pbar_c->completed_tests++;

    if (pbar_c->completed_tests == pbar_c->total_tests) {
        char pbar[TESTING_CHUNIT_PBAR_WIDTH + 1];

        // Manually fill entire progress bar.
        uint16_t i;
        for (i = 0; i < TESTING_CHUNIT_PBAR_WIDTH; i++) {
            pbar[i] = TESTING_CHUNIT_PBAR_CHAR;    
        }

        pbar[TESTING_CHUNIT_PBAR_WIDTH] = '\0';

        printf(TESTING_CHUNIT_PBAR_FMT 
                " Done" CC_RESET "\n", pbar_c->mod->name, pbar);
    }
}

static const chunit_test_decorator CHUNIT_PBAR_TEST_DEC = {
    .start_test = chunit_pbar_start_test,
    .end_test = chunit_pbar_end_test
};

static void *chunit_pbar_get_test_context(const chunit_test_suite *suite, 
        uint64_t i, void *suite_context) {
    // Here we are just passing along the context from the module.
    return suite_context;
}
 
static void chunit_pbar_cleanup_test_context(void *test_context) {
    // Same thing here, nothing to be done.
}

static const chunit_test_suite_decorator CHUNIT_PBAR_TEST_SUITE_DEC = {
    .test_decorator = &CHUNIT_PBAR_TEST_DEC,

    .get_test_context = chunit_pbar_get_test_context,
    .cleanup_test_context = chunit_pbar_cleanup_test_context,
};


static void *chunit_pbar_get_suite_context(const chunit_test_module *mod,
        uint64_t i, void *mod_context) {
    chunit_pbar_context *pbar_c = (chunit_pbar_context *)mod_context; 
    pbar_c->suite_i = i;    // Here just update the suite index.
    return pbar_c; 
}

static void chunit_pbar_cleanup_suite_context(void *suite_context) {
    // Don't need to do anything here!
}

static const chunit_test_module_decorator CHUNIT_PBAR_MOD_DEC = {
    .suite_decorator = &CHUNIT_PBAR_TEST_SUITE_DEC,

    .get_suite_context = chunit_pbar_get_suite_context,
    .cleanup_suite_context = chunit_pbar_cleanup_test_context,
};

chunit_test_module_run *chunit_run_module_pb(const chunit_test_module *mod) {
    chunit_pbar_context *mod_context = 
        safe_malloc(MEM_CHNL_TESTING, sizeof(chunit_pbar_context));

    mod_context->mod = mod;
    mod_context->suite_i = 0;           // This will be written over.
    mod_context->completed_tests = 0;   // This will be incrememnted.
    mod_context->total_tests = 0;

    uint64_t i;
    for (i = 0; i < mod->suites_len; i++) {
        mod_context->total_tests += mod->suites[i]->tests_len;
    }

    chunit_test_module_run *tmr = 
        chunit_run_module(mod, &CHUNIT_PBAR_MOD_DEC, mod_context);

    safe_free(mod_context);

    return tmr;
}

