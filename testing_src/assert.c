#include "./assert.h"
#include "./chunit.h"
#include "../core_src/sys.h"
#include "../core_src/mem.h"

#include <string.h>

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

void chunit_child_process(int fds[2], const chunit_test *test) {
    // First, close the read end of the pipe.
    if (safe_close(fds[0])) {
        exit(CHUNIT_PIPE_ERROR_EXIT_CODE);
    }

    // Get write side of pipe.
    int pipe_fd = fds[1];

    // Run the test.
    test->t(pipe_fd);
    
    // NOTE here we check for memory leaks.
    // If there was non testing memory used before forking,
    // it will always cause a memory leak here.
    if (check_memory_leaks()) {
        write_result(pipe_fd, CHUNIT_MEMORY_LEAK);
    } else {
        write_result(pipe_fd, CHUNIT_SUCCESS);
    }

    close_pipe_and_exit(pipe_fd);
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

    size_t sizes[2] = {strlen(expected) + 1, strlen(actual) + 1};

    // Write both string sizes first.
    write_data(pipe_fd, sizes, sizeof(size_t) * 2);

    // Then write both strings.
    write_data(pipe_fd, expected, sizes[0]);
    write_data(pipe_fd, actual, sizes[1]);

    close_pipe_and_exit(pipe_fd);
}
