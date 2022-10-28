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
#define CHUNIT_PIPE_ERROR_EXIT_CODE 3

// 5 second timeout for all tests.
#define CHUNIT_TIMEOUT_S 5

typedef enum {
    // To be communicated back to parent 
    // when all goes as expected.
    CHUNIT_SUCCESS,

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

    // NOTE, some comm tags will never be 
    // directly from child to parent.
    // They instead will be used by the
    // parent after receiving info from
    // the child. 
    CHUNIT_PIPE_ERROR,
    CHUNIT_FORK_ERROR,
    CHUNIT_TIMEOUT,
    CHUNIT_FATAL_ERROR,
} chunit_comm_tag;

void assert_true(int pipe_fd, int actual);
void assert_false(int pipe_fd, int actual);
void assert_non_null(int pipe_fd, void *ptr);

void assert_eq_ptr(int pipe_fd, void *expected, void *actual);
void assert_eq_int(int pipe_fd, int64_t expected, int64_t actual);
void assert_eq_uint(int pipe_fd, uint64_t expected, uint64_t actual);
void assert_eq_char(int pipe_fd, char expected, char actual);
void assert_eq_str(int pipe_fd, char *expected, char *actual);

// Parent process organization...

typedef struct {
    const char *name;

    // NOTE, this will be the runnable test itself.
    // It must be given a pipe file descriptor
    // to be able to communicate with the parent.
    void (*t)(int pipe_fd);
} chunit_test;

typedef union {
    void       *ptr_c[2];
    int64_t     int_c[2];
    uint64_t    uint_c[2];
    char        char_c[2];
    char       *str_c[2]; 
} chunit_cmpr;

typedef struct {
    chunit_comm_tag tag;

    // Can be whatever is needed...
    // I think this will always point to a compare
    // object tho.
    void *data;
} chunit_test_result;

chunit_test_result *run_test(const chunit_test *test);

#endif
