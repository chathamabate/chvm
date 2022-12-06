#include "./assert.h"
#include "./chunit.h"
#include "../core_src/sys.h"
#include "../core_src/mem.h"
#include "../core_src/io.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>

static inline void test_lock(chunit_test_context *tc) {
    if (pthread_mutex_lock(&(tc->mut))) {
        safe_exit(CHUNIT_MUTEX_ERROR_EXIT_CODE);
    }
}

// NOTE: Notice how there is no unlock function.
// If we are locking on the mutex, this means we are about to
// communicate with the parent, then exit.
// Which ever thread is exiting will die with the lock held.

static void write_result(int pipe_fd, const chunit_test_result res) {
    // Attempt to write the tag to the pipe...
    // if htere is some error, exit with a pipe error.
    if (safe_write(pipe_fd, &res, sizeof(chunit_test_result))) {
        // NOTE, since there has been some sort of pipe error...
        // we won't even bother to close the pipe... just
        // exit.
        
        safe_exit(CHUNIT_PIPE_ERROR_EXIT_CODE);
    }
}

static void write_data(int pipe_fd, void *buf, size_t size) {
    if (safe_write(pipe_fd, buf, size)) {
        safe_exit(CHUNIT_PIPE_ERROR_EXIT_CODE);
    } 
}

static void cleanup_and_exit(chunit_test_context *tc) {
    // Normally, this code below would be a good move.
    // However, threads will be locking on this mutex
    // at the time this function is called.
    //
    // if (pthread_mutex_destroy(&(tc->mut))) {
    //     safe_exit(CHUNIT_MUTEX_ERROR_EXIT_CODE);
    // }

    if (safe_close(tc->pipe_fd)) {
        safe_exit(CHUNIT_PIPE_ERROR_EXIT_CODE);
    }

    safe_exit(0);
}

void chunit_child_process(int fds[2], const chunit_test *test) {
    // NOTE: tests will never print to STDOUT unless specifically
    // asked to do so. I'd recommend never calling fork within
    // a test.
    set_core_quiet(1);

    chunit_test_context tc;

    if (pthread_mutex_init(&(tc.mut), NULL)) {
        safe_exit(CHUNIT_MUTEX_ERROR_EXIT_CODE);
    }

    // First, close the read end of the pipe.
    if (safe_close(fds[0])) {
        safe_exit(CHUNIT_PIPE_ERROR_EXIT_CODE);
    }

    // Get write side of pipe.
    tc.pipe_fd = fds[1];

    // Run the test.
    test->t(&tc);
    
    // NOTE here we check for memory leaks.
    // If there was non testing memory used before forking,
    // it will always cause a memory leak here.
    if (check_memory_leaks(MEM_CHNL_TESTING + 1)) {
        write_result(tc.pipe_fd, CHUNIT_MEMORY_LEAK);
    } else {
        write_result(tc.pipe_fd, CHUNIT_SUCCESS);
    }

    cleanup_and_exit(&tc);
}

void assert_true(chunit_test_context *tc, int actual) {
    if (actual) {
        return;
    }

    test_lock(tc);

    write_result(tc->pipe_fd, CHUNIT_ASSERT_TRUE_FAIL);
    cleanup_and_exit(tc);
}

void assert_false(chunit_test_context *tc, int actual) {
    if (!actual) {
        return;
    }

    test_lock(tc);

    write_result(tc->pipe_fd, CHUNIT_ASSERT_FALSE_FAIL);
    cleanup_and_exit(tc);
}

void assert_non_null(chunit_test_context *tc, void *ptr) {
    if (ptr) {
        return;
    }

    test_lock(tc);

    write_result(tc->pipe_fd, CHUNIT_ASSERT_NON_NULL_FAIL);
    cleanup_and_exit(tc);
}

void assert_eq_ptr(chunit_test_context *tc, void *expected, void *actual) {
    if (expected == actual) {
        return;
    }

    test_lock(tc);

    write_result(tc->pipe_fd, CHUNIT_ASSERT_EQ_PTR_FAIL);

    void *buf[2] = {expected, actual};
    write_data(tc->pipe_fd, buf, sizeof(void *) * 2);
    cleanup_and_exit(tc);
}

void assert_eq_int(chunit_test_context *tc, int64_t expected, int64_t actual) {
    if (expected == actual) {
        return; 
    }

    test_lock(tc);

    write_result(tc->pipe_fd, CHUNIT_ASSERT_EQ_INT_FAIL);

    int64_t buf[2] = {expected, actual};
    write_data(tc->pipe_fd, buf, sizeof(int64_t) * 2);
    cleanup_and_exit(tc);
}

void assert_eq_uint(chunit_test_context *tc, uint64_t expected, uint64_t actual) {
    if (expected == actual) {
        return; 
    }

    test_lock(tc);

    write_result(tc->pipe_fd, CHUNIT_ASSERT_EQ_UINT_FAIL);

    uint64_t buf[2] = {expected, actual};
    write_data(tc->pipe_fd, buf, sizeof(uint64_t) * 2);
    cleanup_and_exit(tc);
}

void assert_eq_char(chunit_test_context *tc, char expected, char actual) {
    if (expected == actual) {
        return; 
    }

    test_lock(tc);

    write_result(tc->pipe_fd, CHUNIT_ASSERT_EQ_CHAR_FAIL);

    char buf[2] = {expected, actual};
    write_data(tc->pipe_fd, buf, sizeof(char) * 2);
    cleanup_and_exit(tc);
}

void assert_eq_str(chunit_test_context *tc, char *expected, char *actual) {
    if (strcmp(expected, actual) == 0) {
        return;
    }

    test_lock(tc);

    write_result(tc->pipe_fd, CHUNIT_ASSERT_EQ_STR_FAIL);
    
    // NOTE, strings send by having the length be 
    // sent first, then the full string...

    size_t sizes[2] = {strlen(expected) + 1, strlen(actual) + 1};

    // Write both string sizes first.
    write_data(tc->pipe_fd, sizes, sizeof(size_t) * 2);

    // Then write both strings.
    write_data(tc->pipe_fd, expected, sizes[0]);
    write_data(tc->pipe_fd, actual, sizes[1]);

    cleanup_and_exit(tc);
}
