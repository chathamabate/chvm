#ifndef TESTING_CHUNIT_H
#define TESTING_CHUNIT_H

#include <stdint.h>

// A unit test will pipe back to the parent process
// information about its execution.
//
// Primarily Assertion Errors with data...

// If there is some form of assertion error...
// The information must be piped back to the parent process.

// What should happen here????
// What does a testing suite even look like???
//
// Hmmmmm... how should this be handled.
//

typedef struct {
    const char *name;
    void (*runnable)(int pipe_fd);   
} chunit_test;

// Code to make assertions work...

// A communication tag will signal
// to the parent process exactly
// what to look for in the pipe.
typedef enum {
    CHUNIT_ASSERT_TRUE,
    CHUNIT_ASSERT_FALSE,
    CHUNIT_ASSERT_NON_NULL,
    CHUNIT_ASSERT_EQ_PTR,
    CHUNIT_ASSERT_EQ_INT,
    CHUNIT_ASSERT_EQ_UINT,
    CHUNIT_ASSERT_EQ_CHAR,
    CHUNIT_ASSERT_EQ_STR
} chunit_assert_tag;

// Some sort of test result union...
// What about for strings tho???
// Maybe a max string length...

typedef struct {
    void *expected;
    void *actual;
} chunit_ptr_cmpr;

typedef struct {
    int64_t expected;
    int64_t actual; 
} chunit_int_cmpr;

typedef struct {
    uint64_t expected;
    uint64_t actual; 
} chunit_uint_cmpr;

typedef struct {
    char expected;
    char actual;
} chunit_char_cmpr;

typedef struct {
    char *expected;
    char *actual;
} chunit_str_cmpr;

typedef union {
    chunit_ptr_cmpr ptr_cmpr;
    chunit_int_cmpr int_cmpr;
    chunit_uint_cmpr uint_cmpr;
    chunit_char_cmpr char_cmpr;
    chunit_str_cmpr str_cmpr;
} chunit_cmpr;

typedef struct {
    chunit_assert_tag assert_tag;
    chunit_cmpr cmpr;
} chunit_assertion_failure;

typedef enum {
    CHUNIT_SUCCESS,
    CHUNIT_ASSERTION_ERROR,
    CHUNIT_TIMEOUT,
    CHUNIT_FATAL_ERROR
} chunit_status;

typedef struct {
    chunit_status s;
    
    // This will only be non null if s = CHUNIT_ASSERTION_ERROR.
    chunit_assertion_failure *assertion_failure;
} chunit_result;

// This will create a new chunit result object
// in dynamic memory.
chunit_result *chunit_run_test(const chunit_test *test);
void delete_chunit_result(chunit_result *res);

void assert_true(int pipe_fd, int actual);
void assert_false(int pipe_fd, int actual);
void assert_non_null(int pipe_fd, void *ptr);

void assert_eq_ptr(int pipe_fd, void *expected, void *actual);

void assert_eq_int(int pipe_fd, int64_t expected, int64_t actual);
void assert_eq_uint(int pipe_fd, uint64_t expected, uint64_t actual);

void assert_eq_char(int pipe_fd, char expected, char actual);

void assert_eq_str(int pipe_fd, char *expected, char *actual);

#endif
