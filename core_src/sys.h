#ifndef CORE_SYS_H
#define CORE_SYS_H

#include "data.h"
#include <sys/_types/_ssize_t.h>
#include <unistd.h>
// Here are some wrappers on popular system functions.
// The intention is to help simplify error handling.

// NOTE : 
// Here are some notes on what errors to look out for
// when using system calls. (i.e. which do we need
// to prepare for, and which do we not.)
//
// * It seems that by default all signals either terminate
// the running process do nothing (ignored)
// The only mysterious one would be SIGSTP and SIGCCONT
// for stopping and continuing a process.
// Here, we would expect that the system call would be interrupted
// and then resumed in some special way.
//
// For all the following calls, I will assume interruption is possible
// and that SA_RESTART is not default for any signal handlers.
// By interruption is possible, I mean that it is possible a system
// call is interrupted and then resumed (instead of the process just
// being terminated)

// NOTE : 
// I will be adding a safe fork as well as a global core state.
// This will be for using fork specifically.
// The global core state will keep track of all processes which
// have been spawned by this process.
// When exiting, this process will always attempt to kill all children.
// Any errors will be noted and printed so that the user knows
// what processes to check for.
//
// The point of this addition is to allow for exiting from 
// the parent process at any point without worrying about
// non terminating child processes.

typedef enum {
    CORE_CHILD_UNREAPED = 0,
    CORE_CHILD_REAPED,
    CORE_CHILD_KILL_OR_REAP_ERROR,
} core_child_status;

typedef struct {

} core_child_state;

typedef struct {
    // This will be a list of children which are yet
    // to be killed and reaped.
    slist *children;   

    // What should happen if there is an error terminating 
    // earlier in the program???
} core_state;

// Will return -1 on error, reaped pid on success.
pid_t safe_waitpid(pid_t pid, int *stat_loc, int opts);

// This will use safe_waitpid with no hang to only wait for a certain
// amount of time.
//
// Any sort of hard error returns -1.
// A timeout returns -2.
// Otherwise, you get back the normal waitpid respose.
pid_t safe_waitpid_timed(pid_t pid, int *stat_loc, time_t timeout);

//  Will attempt to kill the given process then subsequently reap
//  it.
//
//  returns 0 on success, -1 on error.
int safe_kill_and_reap(pid_t pid);

//  Will return -1 on error, 0 on success.
//
//  NOTE : read will return -1 if EOF is found before cnt
//  number of bytes is found!
int safe_write(int fd, const void *buf, size_t cnt);
int safe_read(int fd, void *buf, size_t cnt);
int safe_close(int fd);


#endif
