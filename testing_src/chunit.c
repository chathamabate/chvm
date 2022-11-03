#include "chunit.h"

#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "../core_src/mem.h"
#include "../core_src/sys.h"

static void write_result(int pipe_fd, const chunit_test_result res) {
    // Attempt to write the tag to the pipe...
    // if htere is some error, exit with a pipe error.
    if (safe_write(pipe_fd, &res, sizeof(chunit_test_result))) {
        // NOTE, since there has been some sort of pipe error...
        // we won't even bother to close the pipe... just
        // exit.
        
        exit(CHUNIT_PIPE_ERROR_EXIT_CODE);
    }
}

static void write_data(int pipe_fd, void *buf, size_t size) {
    if (safe_write(pipe_fd, buf, size)) {
        exit(CHUNIT_PIPE_ERROR_EXIT_CODE);
    } 
}

static void close_pipe_and_exit(int pipe_fd) {
    if (safe_close(pipe_fd)) {
        exit(CHUNIT_PIPE_ERROR_EXIT_CODE);
    }

    exit(0);
}

void assert_true(int pipe_fd, int actual) {
    if (actual) {
        return;
    }

    write_result(pipe_fd, CHUNIT_ASSERT_TRUE_FAIL);
    close_pipe_and_exit(pipe_fd);
}

void assert_false(int pipe_fd, int actual) {
    if (!actual) {
        return;
    }

    write_result(pipe_fd, CHUNIT_ASSERT_FALSE_FAIL);
    close_pipe_and_exit(pipe_fd);
}

void assert_non_null(int pipe_fd, void *ptr) {
    if (ptr) {
        return;
    }

    write_result(pipe_fd, CHUNIT_ASSERT_NON_NULL_FAIL);
    close_pipe_and_exit(pipe_fd);
}

void assert_eq_ptr(int pipe_fd, void *expected, void *actual) {
    if (expected == actual) {
        return;
    }

    write_result(pipe_fd, CHUNIT_ASSERT_EQ_PTR_FAIL);

    void *buf[2] = {expected, actual};
    write_data(pipe_fd, buf, sizeof(void *) * 2);
    close_pipe_and_exit(pipe_fd);
}

void assert_eq_int(int pipe_fd, int64_t expected, int64_t actual) {
    if (expected == actual) {
        return; 
    }

    write_result(pipe_fd, CHUNIT_ASSERT_EQ_INT_FAIL);

    int64_t buf[2] = {expected, actual};
    write_data(pipe_fd, buf, sizeof(int64_t) * 2);
    close_pipe_and_exit(pipe_fd);
}

void assert_eq_uint(int pipe_fd, uint64_t expected, uint64_t actual) {
    if (expected == actual) {
        return; 
    }

    write_result(pipe_fd, CHUNIT_ASSERT_EQ_UINT_FAIL);

    uint64_t buf[2] = {expected, actual};
    write_data(pipe_fd, buf, sizeof(uint64_t) * 2);
    close_pipe_and_exit(pipe_fd);
}

void assert_eq_char(int pipe_fd, char expected, char actual) {
    if (expected == actual) {
        return; 
    }

    write_result(pipe_fd, CHUNIT_ASSERT_EQ_CHAR_FAIL);

    char buf[2] = {expected, actual};
    write_data(pipe_fd, buf, sizeof(char) * 2);
    close_pipe_and_exit(pipe_fd);
}

void assert_eq_str(int pipe_fd, char *expected, char *actual) {
    if (strcmp(expected, actual) == 0) {
        return;
    }

    write_result(pipe_fd, CHUNIT_ASSERT_EQ_STR_FAIL);
    
    // NOTE, strings send by having the length be 
    // sent first, then the full string...
    // NO null terminators.
    
    size_t len = strlen(expected);
    write_data(pipe_fd, &len, sizeof(size_t));
    write_data(pipe_fd, expected, len);

    len = strlen(actual);
    write_data(pipe_fd, &len, sizeof(size_t));
    write_data(pipe_fd, actual, len);

    close_pipe_and_exit(pipe_fd);
}

// Parent process Code V2!

// PARENT PROCESS CODE V1!
chunit_test_run *new_test_run() {
    chunit_test_run *tr = safe_malloc(MEM_CHNL_TESTING, sizeof(chunit_test_run));
    tr->errors = new_slist(MEM_CHNL_TESTING, sizeof(chunit_framework_error));

    tr->result = CHUNIT_VOID;
    tr->data = NULL;

    return tr;
}

chunit_test_run *new_test_error(chunit_framework_error err) {
    chunit_test_run *tr = new_test_run();
    *(chunit_framework_error *)sl_next(tr->errors) = err;
    return tr;
}

void delete_test_run() {
    // TODO
}


// NOTE fds[1] = writing descriptor.
// fds[0] = reading descriptor.

static void chunit_child_process(int fds[2], const chunit_test *test) {
    // First, close the read end of the pipe.
    if (safe_close(fds[0])) {
        exit(CHUNIT_PIPE_ERROR_EXIT_CODE);
    }

    // Get write side of pipe.
    int pipe_fd = fds[1];

    // Run the test.
    test->t(pipe_fd);

    // If we've made it here, success!
    write_result(pipe_fd, CHUNIT_SUCCESS);
    close_pipe_and_exit(pipe_fd);
}

static chunit_test_run *chunit_parent_process(int fds[2], pid_t child) {
    // Attempt to close write end of pipe.
    if (safe_close(fds[1])) {
        chunit_test_run *ce_tr = new_test_error(CHUNIT_PIPE_ERROR);

        // When there is an error killing the child,
        // make sure to add in a termination error.
        if (safe_kill_and_reap(child)) {
            *(chunit_framework_error *)sl_next(ce_tr->errors) = 
                CHUNIT_TERMINATION_ERROR;
        }

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
        return new_test_error(CHUNIT_TERMINATION_ERROR);
    }

    if (res == -2) {
        // A timeout has occurred.
        // Try to kill and reap the process.
        if (safe_kill_and_reap(child)) {
            return new_test_error(CHUNIT_TERMINATION_ERROR);
        }

        // If no error in termination, report a timeout.
        chunit_test_run *timeout_tr = new_test_run();
        timeout_tr->result = CHUNIT_TIMEOUT;

        return timeout_tr;
    }

    // Here the program terminated on its own...
    // check to see if it errored out.

    // If the child didn't exit normally, return a fatal
    // error. (User's fault)
    if (!WIFEXITED(stat)) {
        // Otherwise, their error.
        chunit_test_run *fatal_tr = new_test_run(); 
        fatal_tr->result = CHUNIT_FATAL_ERROR;
        return fatal_tr;
    }

    // If we make it here, we must've exited normally.

    // Check if there was some pipe error. (OUR ERROR)
    if (WEXITSTATUS(stat) == CHUNIT_PIPE_ERROR_EXIT_CODE) {
        return new_test_error(CHUNIT_PIPE_ERROR);
    }

    // This means the process terminated on it's own
    // and it wasn't some fatal error/pipe error.
    // Now we must interpret the results.
    
    

    return NULL;
}

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

/*
// NOTE consider making this a core function some day.
static void chunit_kill_child_and_wait(pid_t pid) {
    // Killing the child process should always work.
    kill(pid, SIGKILL);

    // Keep waiting until we get this fucker.
    while (waitpid(pid, NULL, 0) == -1);
}

static void chunit_parent_process(int fds[2], pid_t pid, chunit_test_result *result) {
    // NOTE errors in parent process should trigger killing the child process.
    result->data = NULL;
    
    // First attempt to close writing pipe.
    if (close(fds[1])) {
        result->tag = CHUNIT_PIPE_ERROR;
        chunit_kill_child_and_wait(pid);

        // finally, return.
        return;
    }

    int stat;
    time_t start = time(NULL); // This will be in seconds.

    // First we wait for the process to exit.
    while (1) {
        pid_t ret_pid;
        while ((ret_pid = waitpid(pid, &stat, WNOHANG)) == -1);
        
        // Here the child process has ended! 
        if (ret_pid) {
            break;
        }

        // If the child process has not ended, let's
        // make sure we haven't timed out.
        time_t curr = time(NULL);
        if (curr - start >= CHUNIT_TIMEOUT_S) {
            // Time for a timeout my friend.
            result->tag = CHUNIT_TIMEOUT;
            chunit_kill_child_and_wait(pid);
            return;
        }

        // Otherwise... lets sleep for a bit and come back later.
        usleep(CHUNIT_SLEEP_TIME_MS * 1000);
    }
    
    // At this point, our process has exited!
   
}

static void chunit_child_process(int fds[2], const chunit_test *test) {
    // In the child process, we must close the read end of the pipe.
    if (close(fds[0])) {
        // Pipe close error. Forget about the pipe, just exit.
        exit(CHUNIT_PIPE_ERROR_EXIT_CODE);
    }     

    int pipe_fd = fds[1];
    test->t(pipe_fd);

    // NOTE if we've made it here, all assertions must've been correct.
    // Now, we make sure all memory channels besides the testing 
    // channel have count zero.
    if (check_memory_leaks()) {
        write_tag(pipe_fd, CHUNIT_MEMORY_LEAK);
    }

    // Finally, just close and exit my friend.
    close_pipe_and_exit(pipe_fd);
}

chunit_test_result *run_test(const chunit_test *test) {
    // Only ever dynamically make space for this in the parent process.
    chunit_test_result *result;

    // First set up communication pipe.
    int fds[2];
    if (pipe(fds) == -1) {
        result = safe_malloc(MEM_CHNL_TESTING, sizeof(chunit_test_result));
        result->tag = CHUNIT_PIPE_ERROR;
        result->data = NULL;

        return result;
    }

    // Time to fork my friend...
    pid_t pid = fork();

    if (pid == -1)  {
        result = safe_malloc(MEM_CHNL_TESTING, sizeof(chunit_test_result));
        result->tag = CHUNIT_FORK_ERROR;
        result->data = NULL;

        // Attempt to close pipe...
        // error here will not take precedence over
        // fork error.
        close(fds[0]);
        close(fds[1]);

        return result;
    } 

    if (pid == 0) {
        chunit_child_process(fds, test);

        // Should never make it to this line.
        return NULL; 
    }

    result = safe_malloc(MEM_CHNL_TESTING, sizeof(chunit_test_result));
    chunit_parent_process(fds, pid, result);

    return result;
}*/
