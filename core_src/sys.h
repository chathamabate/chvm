#ifndef CORE_SYS_H
#define CORE_SYS_H

#include "./log.h"
#include <stdint.h>
#include <sys/_pthread/_pthread_mutex_t.h>
#include <sys/_pthread/_pthread_rwlock_t.h>
#include <sys/_types/_pid_t.h>
#include <sys/_types/_sigset_t.h>
#include <sys/_types/_ssize_t.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

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
//
// NOTE : 
// All system call wrappers will be atomic on the core state
// that they modify.
//
// NOTE :
// I did not anticpate the POSIX standard for how threads propegate
// when a process is forked. So DO NOT use safe fork when using a
// process which has already spawned multiple threads.
// Always do threading after forking... 
// If you do decide to fork a multi threaded program, the behavoir
// is undefined and probably UNSAFE.

typedef struct child_list_struct child_list;

typedef struct {
    // This is whether or not we are working with the
    // root process... Only the root process will be allowed
    // to fork.
    uint8_t root;

    // Whether or not this process should output.
    uint8_t quiet;

    // Lock to make all operations on core_state
    // atomic.
    pthread_rwlock_t core_lock;

    // This will be a list of children which are yet
    // to be killed and reaped on exit.
    child_list *children;

    uint8_t num_mem_chnls;
    uint64_t *mem_chnls;
} core_state;

// NOTE here is a global variable which will be used
// throughout program use.
//
// The user should NEVER access this directly.
extern core_state *_core_state;

// Give us the number of mem chanels to use.
// NOTE, this must be called before using any other system
// calls wrappers. Additionally, this should never be 
// called in a child process.
//
// This will exit if there is an error setting up
// the core.
void init_core_state(uint8_t nmcs);

// NOTE the core state has an elaborate sigint handler
// which prints output, these calls are used to block
// SIGINT in times when reentrance must be avoided.
void _block_sigint(sigset_t *old);

static inline void _restore_sigmask(sigset_t *old) {
    pthread_sigmask(SIG_SETMASK, old, NULL);
}

// The lock wrappers on core will also block SIGINT.
// This way, this way SIGINT will never interrupt a thread
// which is working on the core state.
void _rdlock_core_state();
void _wrlock_core_state();
void _unlock_core_state();

// 1 for quiet, 0 for loud.
void set_core_quiet(uint8_t q);

// Logging function used by core and accessable to
// the user!
//
// lck is whether or not the lock must be aquired.
// If we arlready have the lock (in read or write mode)
// lck should be 0, 1 otherwise.
void core_logf(uint8_t lck, const char *fmt, ...);

// This will fork and record the child's pid in the
// _core_state.
int safe_fork();

// Safe exit should always be called over normal exit
// once the core has been set up.
// It will kill all child processes before exiting.
// 
// q will be whether or not notes should be logged
// during exit. (i.e which child processes were killed
// and if there were memory leaks)
void safe_exit(int code);

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

#endif
