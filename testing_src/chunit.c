#include "chunit.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../core_src/mem.h"



static void write_tag(int pipe_fd, const chunit_comm_tag tag) {
    // Attempt to write the tag to the pipe...
    // if htere is some error, exit with a pipe error.
    if (write(pipe_fd, &tag, sizeof(chunit_comm_tag)) != sizeof(chunit_comm_tag)) {
        // NOTE, since there has been some sort of pipe error...
        // we won't even bother to close the pipe... just
        // exit.
        
        exit(CHUNIT_PIPE_ERROR);
    }
}

static void write_data(int pipe_fd, void *buf, size_t size) {
    if (write(pipe_fd, buf, size) != size) {
        exit(CHUNIT_PIPE_ERROR);
    } 
}

static void close_pipe_and_exit(int pipe_fd) {
    if (close(pipe_fd)) {
        // NOTE if there is any form of pipe error,
        // this should take precedence over any sort of 
        // soft error.
        exit(CHUNIT_PIPE_ERROR);
    }

    exit(0);
}

void assert_true(int pipe_fd, int actual) {
    if (actual) {
        return;
    }

    write_tag(pipe_fd, CHUNIT_ASSERT_TRUE_FAIL);
    close_pipe_and_exit(pipe_fd);
}

void assert_false(int pipe_fd, int actual) {
    if (!actual) {
        return;
    }

    write_tag(pipe_fd, CHUNIT_ASSERT_FALSE_FAIL);
    close_pipe_and_exit(pipe_fd);
}

void assert_non_null(int pipe_fd, void *ptr) {
    if (ptr) {
        return;
    }

    write_tag(pipe_fd, CHUNIT_ASSERT_NON_NULL_FAIL);
    close_pipe_and_exit(pipe_fd);
}

void assert_eq_ptr(int pipe_fd, void *expected, void *actual) {
    if (expected == actual) {
        return;
    }

    write_tag(pipe_fd, CHUNIT_ASSERT_EQ_PTR_FAIL);

    void *buf[2] = {expected, actual};
    write_data(pipe_fd, buf, sizeof(void *) * 2);
    close_pipe_and_exit(pipe_fd);
}

void assert_eq_int(int pipe_fd, int64_t expected, int64_t actual) {
    if (expected == actual) {
        return; 
    }

    write_tag(pipe_fd, CHUNIT_ASSERT_EQ_INT_FAIL);

    int64_t buf[2] = {expected, actual};
    write_data(pipe_fd, buf, sizeof(int64_t) * 2);
    close_pipe_and_exit(pipe_fd);
}

void assert_eq_uint(int pipe_fd, uint64_t expected, uint64_t actual) {
    if (expected == actual) {
        return; 
    }

    write_tag(pipe_fd, CHUNIT_ASSERT_EQ_UINT_FAIL);

    uint64_t buf[2] = {expected, actual};
    write_data(pipe_fd, buf, sizeof(uint64_t) * 2);
    close_pipe_and_exit(pipe_fd);
}

void assert_eq_char(int pipe_fd, char expected, char actual) {
    if (expected == actual) {
        return; 
    }

    write_tag(pipe_fd, CHUNIT_ASSERT_EQ_CHAR_FAIL);

    char buf[2] = {expected, actual};
    write_data(pipe_fd, buf, sizeof(char) * 2);
    close_pipe_and_exit(pipe_fd);
}

void assert_eq_str(int pipe_fd, char *expected, char *actual) {
    if (strcmp(expected, actual) == 0) {
        return;
    }

    write_tag(pipe_fd, CHUNIT_ASSERT_EQ_STR_FAIL);
    
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
