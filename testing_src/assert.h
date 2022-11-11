#ifndef TESTING_ASSERT_H
#define TESTING_ASSERT_H

#include <stdint.h>
#include "./chunit.h"

void chunit_child_process(int fds[2], const chunit_test *test);

// Below are the only chunit calls which do not
// come with chunit prefix.
void assert_true(int pipe_fd, int actual);
void assert_false(int pipe_fd, int actual);
void assert_non_null(int pipe_fd, void *ptr);

void assert_eq_ptr(int pipe_fd, void *expected, void *actual);
void assert_eq_int(int pipe_fd, int64_t expected, int64_t actual);
void assert_eq_uint(int pipe_fd, uint64_t expected, uint64_t actual);
void assert_eq_char(int pipe_fd, char expected, char actual);
void assert_eq_str(int pipe_fd, char *expected, char *actual);

#endif
