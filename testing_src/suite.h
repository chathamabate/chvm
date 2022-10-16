#ifndef TESTING_SUITE_H
#define TESTING_SUITE_H

// How should tests look???
// Should logging be a universal thing???
// What should tests return... a list of issues....


typedef enum {
    OUTPUT_STYLE_BRIEF,
    OUTPUT_STYLE_VERBOSE 
} output_style;

typedef enum {
    TEST_PASS,
    TEST_FAIL   
} test_result;

typedef struct {
    const char *name;
    test_result (*test)(output_style);
} test_case;

test_result run_test_case(output_style os, const test_case *tc);

#endif
