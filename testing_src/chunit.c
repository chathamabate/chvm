#include "./chunit.h"

#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "../core_src/mem.h"
#include "../core_src/sys.h"
#include "./assert.h"

// Parent process and Child process code for CHUNIT.

chunit_test_run *new_test_run() {
    chunit_test_run *tr = safe_malloc(MEM_CHNL_TESTING, sizeof(chunit_test_run));
    tr->errors = new_slist(MEM_CHNL_TESTING, sizeof(chunit_framework_error));

    tr->result = CHUNIT_VOID;
    tr->data = NULL;

    return tr;
}

chunit_test_run *new_test_result(chunit_test_result res) {
    chunit_test_run *tr = new_test_run();
    tr->result = res;
    return tr;
}

chunit_test_run *new_test_error(chunit_framework_error err) {
    chunit_test_run *tr = new_test_run();
    *(chunit_framework_error *)sl_next(tr->errors) = err;
    return tr;
}

void delete_test_run() {
    // TODO delete test run based on what the result is.
}


// NOTE fds[1] = writing descriptor.
// fds[0] = reading descriptor.
//

static void attempt_safe_kill_and_reap(chunit_test_run *tr, pid_t pid) {
    if (safe_kill_and_reap(pid)) {
        *(chunit_framework_error *)sl_next(tr->errors) = CHUNIT_TERMINATION_ERROR;
    }
}

static void attempt_safe_close(chunit_test_run *tr, int pipe_fd) {
    if (safe_close(pipe_fd)) {
        *(chunit_framework_error *)sl_next(tr->errors) = CHUNIT_PIPE_ERROR;
    }
}

static void attempt_read_cmpr_and_close(chunit_test_run *tr, int pipe_fd, 
        chunit_test_result res, size_t c_size) {
    void *buf = safe_malloc(MEM_CHNL_TESTING, c_size * 2);

    if (safe_read(pipe_fd, buf, c_size * 2)) {
        *(chunit_framework_error *)sl_next(tr->errors) = 
            CHUNIT_PIPE_ERROR;

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

static chunit_test_run *chunit_parent_process(int fds[2], pid_t child) {
    // NOTE : going forward in the parent process... 
    // If there is a non-pipe error, we must close the read end of
    // the pipe before exiting!

    // Attempt to close write end of pipe.
    if (safe_close(fds[1])) {
        chunit_test_run *ce_tr = new_test_error(CHUNIT_PIPE_ERROR);
        attempt_safe_kill_and_reap(ce_tr, child);

        // Since this is a pipe error, don't try to close
        // read end of the pipe.

        return ce_tr;
    }

    // Get read end of the pipe
    int pipe_fd = fds[0];
    int stat;
    
    // Wait for our boy to finish up here!
    pid_t res = safe_waitpid_timed(child, &stat, CHUNIT_TIMEOUT_S); 

    // When there's a hard waitpid error, just return...
    // don't worry about killing.
    if (res == -1) {
        chunit_test_run *we_tr = new_test_error(CHUNIT_TERMINATION_ERROR);
        attempt_safe_close(we_tr, pipe_fd);

        return we_tr;
    }

    // There's been a timeout, kill, reap, close, and return.
    if (res == -2) {
        chunit_test_run *timeout_tr = new_test_result(CHUNIT_TIMEOUT);
        attempt_safe_kill_and_reap(timeout_tr, child);
        attempt_safe_close(timeout_tr, pipe_fd);

        return timeout_tr;
    }

    // Here the program terminated on its own...
    // check to see if it errored out.

    // If the child didn't exit normally, return a fatal
    // error. (User's fault)
    if (!WIFEXITED(stat)) {
        chunit_test_run *f_tr = new_test_result(CHUNIT_FATAL_ERROR);
        attempt_safe_close(f_tr, pipe_fd);

        return f_tr;
    }

    // If we make it here, we must've exited normally.

    // Check if there was some pipe error. (OUR ERROR)
    if (WEXITSTATUS(stat) == CHUNIT_PIPE_ERROR_EXIT_CODE) {
        // pipe doesn't need to be closed.
        return new_test_error(CHUNIT_PIPE_ERROR);
    }

    // This is a positive exit status outside of the pipe error exit code.
    if (WEXITSTATUS(stat) > 0) {
        // Ashamed of the copy and paste which occured here.
        chunit_test_run *f_tr = new_test_result(CHUNIT_FATAL_ERROR);
        attempt_safe_close(f_tr, pipe_fd);

        return f_tr;
    }

    // This means the process terminated on it's own
    // and it wasn't some fatal error/pipe error.
    // Now we must interpret the results.
    
    // TODO : finish result interpretation.
    
    chunit_test_result t_res;
    
    // Read comm tag.
    if (safe_read(pipe_fd, &t_res, sizeof(chunit_test_result))) {
        return new_test_error(CHUNIT_PIPE_ERROR);
    }

    // NOTE, consider creating a results interpretation helper function.

    chunit_test_run *tr = new_test_run();
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
                *(chunit_framework_error *)sl_next(tr->errors) = 
                    CHUNIT_PIPE_ERROR;

                return tr;
            }

            char *expected = safe_malloc(MEM_CHNL_TESTING, str_sizes[0]);

            // Read expected.
            if (safe_read(pipe_fd, expected, str_sizes[0])) {
                *(chunit_framework_error *)sl_next(tr->errors) = 
                    CHUNIT_PIPE_ERROR;

                safe_free(expected);
                return tr;
            }

            char *actual = safe_malloc(MEM_CHNL_TESTING, str_sizes[1]);

            // Read actual.
            if (safe_read(pipe_fd, actual, str_sizes[1])) {
                *(chunit_framework_error *)sl_next(tr->errors) = 
                    CHUNIT_PIPE_ERROR;

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
            *(chunit_framework_error *)sl_next(tr->errors) = CHUNIT_BAD_TEST_RESULT;
            attempt_safe_close(tr, pipe_fd);

            return tr;
    }
}

// NOTE, the pipes should always be closed!!!

chunit_test_run *run_test(const chunit_test *test) {
    int fds[2];

    if (pipe(fds)) {
        return new_test_error(CHUNIT_PIPE_ERROR);
    }

    pid_t pid = fork();

    if (pid == -1) {
        return new_test_error(CHUNIT_FORK_ERROR);
    }

    if (pid == 0) {
        // NOTE, we should not leave this call...
        // i.e. chunit_child_process must exit!
        chunit_child_process(fds, test);    
    }

    return chunit_parent_process(fds, pid);
}

