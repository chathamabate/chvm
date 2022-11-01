#ifndef CORE_SYS_H
#define CORE_SYS_H

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

// No need for a wrapper for the following functions.
// These functions only return -1 in the case of a serious error.
//
// fork, kill, pipe

#endif
