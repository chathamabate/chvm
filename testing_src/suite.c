#include "suite.h"
#include <stdio.h>

test_result run_test_case(output_style os, const test_case *tc) {
    const char *prefix = os == OUTPUT_STYLE_BRIEF 
        ?   "   > %s " 
        :   "   > %s\n";

    printf(prefix, tc->name);

    test_result tr = tc->test(os);
    const char *pass_foot = ANSI_COLOR_GREEN "PASS" ANSI_COLOR_RESET "\n";
    const char *fail_foot = ANSI_COLOR_RED "FAIL" ANSI_COLOR_RESET "\n";
    
    return tr;
}
