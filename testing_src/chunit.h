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

typedef struct {
    const char *name;
    void (*runnable)(void);
} chunit_test;

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

#define CHUNIT_STR_BUF_LEN 25

typedef struct {
    char expected[CHUNIT_STR_BUF_LEN];
    char actual[CHUNIT_STR_BUF_LEN];
} chunit_str_cmpr;

typedef enum {
    CHUNIT_SUCCESS,
    CHUNIT_ASSERTION_ERROR,
    CHUNIT_TIMEOUT,
    CHUNIT_FATAL_ERROR
} chunit_status;

typedef struct {

} chunit_result;

// How should this really run though???
int chunit_run_test(const chunit_test *test);

void assert_true(int actual);
void assert_false(int actual);
void assert_non_null(void *ptr);

void assert_eq_ptr(void *expected, void *actual);

void assert_eq_int(int64_t expected, int64_t actual);
void assert_eq_uint(uint64_t expected, uint64_t actual);

void assert_eq_char(char expected, char actual);

void assert_eq_str(char *expected, char *actual);

#endif
