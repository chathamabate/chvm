#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include "testing_src/chunit.h"
#include "testing_src/assert.h"
#include "core_src/mem.h"
#include "core_src/data.h"
#include "core_src/log.h"
#include "testing_src/output.h"

// This works!!!
//

void test_dummy(int pipe_fd) {
    while(1) {} // SHUH YEH
}

const chunit_test TEST = {
    .name = "Test 1",
    .t = test_dummy
};

int main(void) {
    chunit_test_run *tr = run_test(&TEST);

    print_test_run(tr);

    display_channels();
    delete_test_run(tr);

    return 1;

    // The question becomes.... 
    // What should testing look like?
    // Should there be timeouts?
    // Hard vs Soft errors?
    // How should tests be organized???
    //
    // A module will contain one entry point into
    // its tests.
    // The module should then contain multiple suites
    // all containing individual unit tests.
    //
    // modules -> suites -> tests
    //
    // Each unit test will be forked onto another process
    // This way, one fatal error does not bring down the whole
    // program. 
    //
    // How will tests signal they're return status
    //
    // There are a few cases : (I think these all can be dealt with)
    //      
    //      Handled by child proccess.
    //      1) Success  
    //      2) Soft Error (Assertion error)
    //      
    //      Handled by parent process.
    //      3) Timeout (Presumably infinite loop)
    //      4) Hard Error
    //
    // How should soft errors be dealt with...
    // there needs to be some form of error message ability???
    // How will information be returned to the parent process???
    // All through status codes maybe????
    //
    // We are going to be forking big time!!

    
    pid_t pid = fork();

    if (pid < 0) {
        printf("Failure to create child process\n");
        return 1;
    }

    if (pid == 0) {
        
        int arr[100];
        arr[0] = 0;
        int x = 1 / arr[0]; // This doesn't for some reason.
        printf("%d", x);
        exit(100);

    } else {
        int stat;
        waitpid(pid, &stat, 0);

        if (WIFEXITED(stat)) {
            printf("NORMAL EXIT %d\n", WEXITSTATUS(stat));
        } else {
            printf("EXIT CODE %d\n", WEXITSTATUS(stat));
        }
    }

    return 0;
}
