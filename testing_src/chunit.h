#ifndef TESTING_CHUNIT_H
#define TESTING_CHUNIT_H

#include <stdint.h>

// All errors will be piped back to the parent 
// ... except for when 2 different errors.
// 1) Would be when the test process exits due to a hard error.
// 2) When there is an error communicating with the parent process.
//
// NOTE, In the second case, the process should exit with the below
// error code.
#define CHUNIT_PIPE_ERROR 3

typedef enum {
    // Single arg assertion fails.
    // Just tag will be sent.
    CHUNIT_ASSERT_TRUE_FAIL,
    CHUNIT_ASSERT_FALSE_FAIL,
    CHUNIT_ASSERT_NON_NULL_FAIL,

    // Double arg assertions fails.
    // Tag, expected, actual will be sent.
    CHUNIT_ASSERT_EQ_PTR_FAIL,
    CHUNIT_ASSERT_EQ_INT_FAIL,
    CHUNIT_ASSERT_EQ_UINT_FAIL,
    CHUNIT_ASSERT_EQ_CHAR_FAIL,

    // Tag, expected len, expected, 
    // actual len, actual will be sent.
    CHUNIT_ASSERT_EQ_STR_FAIL,

    // Memory Leak.
    CHUNIT_MEMORY_LEAK,
} chunit_comm_tag;

void assert_true(int pipe_fd, int actual);
void assert_false(int pipe_fd, int actual);
void assert_non_null(int pipe_fd, void *ptr);

void assert_eq_ptr(int pipe_fd, void *expected, void *actual);

void assert_eq_int(int pipe_fd, int64_t expected, int64_t actual);
void assert_eq_uint(int pipe_fd, uint64_t expected, uint64_t actual);

void assert_eq_char(int pipe_fd, char expected, char actual);

void assert_eq_str(int pipe_fd, char *expected, char *actual);

#endif
