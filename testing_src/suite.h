#ifndef TESTING_SUITE_H
#define TESTING_SUITE_H

// Terminal colors.
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

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
