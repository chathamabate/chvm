#ifndef TESTING_CHUNIT_H
#define TESTING_CHUNIT_H

#include <stdint.h>
#include "../core_src/data.h"

// NOTE :
// An assertion failure is not an error.
// It reveals a bug to the user, but it is not
// an error in the testing framework.
//
// However, a pipe error, or an error writing data...
// these are errors in the program.
// Not the fault of the user.
// So, these situations I think should be treated differently.

// A test result is what is interpreted upon completion of a test.
// Completion of a test implies there were no errors in the framework
// when running the test.
typedef enum {
    // This is for when a result was unable to be interpreted.
    CHUNIT_VOID,

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

    // Test took too long.
    CHUNIT_TIMEOUT, 

    // Test had a fatal runtime error.
    CHUNIT_FATAL_ERROR,
} chunit_test_result;

// This exit code should be used when there is a 
// write error from the test to the parent.
#define CHUNIT_PIPE_ERROR_EXIT_CODE 3

// This enum is seeing whether or not an error
// occured in the framework.
typedef enum {
    CHUNIT_PIPE_ERROR,
    CHUNIT_FORK_ERROR,
    CHUNIT_BAD_TEST_RESULT,
    CHUNIT_TERMINATION_ERROR,
} chunit_framework_error;

// 5 second timeout for all tests.
#define CHUNIT_TIMEOUT_S 5

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
    // A list of framework errors.
    slist *errors;

    // Only to be interpreted when there 
    // are no chunit errors.
    chunit_test_result result;

    // Finally, a data pointer to any other information
    // needed to interpret the test run...
    // i.e. assertions comparisons.
    void *data;
} chunit_test_run;

// Test runs are always in the test channel
// of memory.
chunit_test_run *new_test_run();
chunit_test_run *new_test_result(chunit_test_result res);
chunit_test_run *new_test_error(chunit_framework_error err);
void delete_test_run(chunit_test_run *tr);

chunit_test_run *run_test(const chunit_test *test);

#endif
