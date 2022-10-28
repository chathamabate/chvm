#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "testing_src/chunit.h"
#include "core_src/mem.h"

int main(void) {

    int *arr = safe_malloc(5, sizeof(int) * 100);
    arr[0] = 100;
    safe_free(arr); 
    display_channels();
    return 0;
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

    
    int fds[2]; 
    if (pipe(fds) == -1) {
        printf("Error creating pipe\n");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        printf("Failure to create child process\n");
        close(fds[0]);
        close(fds[1]);
        return 1;
    }

    if (pid == 0) {
        // Child proccess.
        close(fds[0]); // Potential error checking.

        write(fds[1], "Hello, World!", 14); 
        close(fds[1]);
    } else {
        // Parent process.
        close(fds[1]);
        // THIS WORKS!!!!
        sleep(5);

        char buff[20];
        read(fds[0], buff, 14); // Read is blocking!
        close(fds[0]);

        printf("%s\n", buff);
    }

    return 0;
}
