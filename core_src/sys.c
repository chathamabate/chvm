#include "./sys.h"
#include "log.h"
#include <pthread.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <inttypes.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdarg.h>
#include "./io.h"

// Below lies a list type which doesn't use any of the core safe
// mechanisms, it is specifically for holding pids and that's it!

#define INITIAL_CHILD_LIST_CAP 1

typedef struct child_list_struct {
    uint64_t len;
    uint64_t cap;
    
    pid_t *buf;
} child_list;

static child_list *new_child_list() {
    child_list *cl = malloc(sizeof(child_list)); 

    if (!cl) {
        return NULL;
    }

    cl->len = 0;
    cl->cap = INITIAL_CHILD_LIST_CAP;

    cl->buf = malloc(sizeof(pid_t) * cl->cap);

    if (!cl->buf) {
        free(cl);
        return NULL;
    }

    return cl;
}

static inline void delete_child_list(child_list *cl) {
    free(cl->buf);
    free(cl);
}

static inline pid_t cl_get(child_list *cl, uint64_t i) {
    return cl->buf[i];
}

// Returns -1 if there was some realloc error.
static int cl_add(child_list *cl, pid_t child) {
    // Attempt to realloc if needed.
    if (cl->len == cl->cap) {
        uint64_t new_cap = cl->cap * 2;

        pid_t *new_buf = realloc(cl->buf, sizeof(pid_t) * new_cap);

        // Realloc error.
        if (!(new_buf)) {
            return -1;
        }

        // Only use the new buffer if it's non NULL!
        cl->buf = new_buf;
        cl->cap = new_cap;
    }

    cl->buf[cl->len++] = child;
    return 0;
}

static void cl_remove(child_list *cl, uint64_t i) {
    if (i >= cl->len) {
        return;
    }

    uint64_t j; // Shift to the left here.
    for (j = i; j < cl->len - 1; j++) {
        cl->buf[j] = cl->buf[j + 1];
    }     

    cl->len--;
}

// NOTE this handler is the sole reason we must
// use safe print everywhere in our program.
//
// Since we redirect to safe_exit, and safe_exit
// prints output, we must make sure to block 
// SIGINT whenever we print else where.
// Otherwise, there would be potential for
// a reentrance error!
static void safe_sigint_handler(int signo) {
    (void)signo;

    safe_printf("\n" CC_ITALIC "Handling SIGINT..." CC_RESET "\n");

    safe_exit(0);
}

core_state *_core_state = NULL;

static inline void init_core_err(const char *msg) {
    printf(CC_BRIGHT_RED "[Core Init Error] " CC_RESET 
            CC_FAINT CC_ITALIC "%s" CC_RESET "\n", msg);
    exit(1);
}

// Give us the number of mem chanels to use.
void init_core_state(uint8_t nmcs) {
    // We will not be using safe malloc until we have the core setup.
    if (!(_core_state = malloc(sizeof(core_state)))) {
        init_core_err("Unable to allocate core state.");
    }

    _core_state->root = 1;
    if (pthread_rwlock_init(&(_core_state->core_lock), NULL)) {
        init_core_err("Unable to create core lock.");
    }

    if (!(_core_state->children = new_child_list())) {
        init_core_err("Unable to allocate child list.");
    }

    // Create mem channels array.
    _core_state->num_mem_chnls = nmcs;
    if (!(_core_state->mem_chnls = malloc(nmcs * sizeof(uint64_t)))) {
        init_core_err("Unable to allocate memory channels.");
    }

    // Finally, install the standard safe sig int handler.
    signal(SIGINT, safe_sigint_handler); 
}

void _block_sigint(sigset_t *old) {
    sigset_t mask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    pthread_sigmask(SIG_BLOCK, &mask, old);
}

// NOTE in the below lock functions it is crucial to
// remember that when we acquire any lock on the core,
// SIGINT is also blocked.
//
// Sometimes, (like when using printf) we must block SIGINT
// but do not need to lock on the core. In this case,
// we should use the functions _block_sigint and _restore_sigmask.

static void edit_sigint(int how) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);

    pthread_sigmask(how, &set, NULL);
}

// NOTE: the below three functions are only be used within
// the core module. Hence the underscore.
// Additionally, they are never to be used in safe exit.
// When we call safe exit, we must alway exit.

void _rdlock_core_state() {
    edit_sigint(SIG_BLOCK);
    if (pthread_rwlock_rdlock(&(_core_state->core_lock))) {
        core_logf(0, "Error while acquiring read lock on core.");
        safe_exit(1);
    }
}

void _wrlock_core_state() {
    edit_sigint(SIG_BLOCK);
    if (pthread_rwlock_wrlock(&(_core_state->core_lock))) {
        core_logf(0, "Error while acquiring write lock on core.");
        safe_exit(1);
    }
}

void _unlock_core_state() {
    // NOTE : The order is essential HERE!
    if (pthread_rwlock_unlock(&(_core_state->core_lock))) {
        core_logf(0, "Error while releasing lock on core.");
        safe_exit(1);
    }
    edit_sigint(SIG_UNBLOCK);
}

void core_logf(uint8_t lck, const char *fmt, ...) {
    // To read the root field, we need to lock and block
    // on the core.
    if (lck) {
        _rdlock_core_state();
    }

    uint8_t root = _core_state->root;

    if (lck) {
        _unlock_core_state();
    }

    va_list args;
    va_start(args, fmt);

    sigset_t old;

    // To just print output, we simply need to block SIGINT.
    if (lck) {
        _block_sigint(&old);
    }

    if (root) {
        printf(CC_BRIGHT_CYAN "[ROOT] " CC_RESET CC_ITALIC CC_FAINT);
    } else {
        printf(CC_BRIGHT_MAGENTA "[%d] " CC_RESET CC_ITALIC CC_FAINT, 
                getpid());
    }

    vprintf(fmt, args);

    printf(CC_RESET "\n");

    if (lck) {
        _restore_sigmask(&old);
    }

    va_end(args);
}

int safe_fork() {
    _wrlock_core_state();

    // For now, we can only fork from the root process.
    if (!(_core_state->root)) {
        _unlock_core_state();
        return -1;
    }

    pid_t fres = fork();

    // Vanilla fork error.
    if (fres == -1) {
        _unlock_core_state();
        return -1;
    }

    // Child process.
    if (fres == 0) {
        _core_state->root = 0; 

        // No children list is needed for the child
        // process.
        delete_child_list(_core_state->children);
        _core_state->children = NULL;
         
        _unlock_core_state();
        return 0;
    }

    // Parent process. Add the child pid to our children list.
    if (cl_add(_core_state->children, fres) == -1) {
        // There's been an error adding the child to the list...
        // what should we do... Maybe just tell the user.
        core_logf(0, "Child process %d was unable to be recorded. Confirm its termination.", 
                fres); 
    }

    _unlock_core_state();
    return fres;
}

void safe_exit_param(int code, uint8_t q) {
    edit_sigint(SIG_BLOCK);

    // If there is an error acquiring the read lock on the core
    // who knows what the core will hold when we read/write to
    // it in the below code.
    // Thus, we will just exit and warn the user about what 
    // happened.
    if (pthread_rwlock_wrlock(&(_core_state->core_lock))) {
        // We have SIGINT blocked, so no need to call safe_printf.
        printf(CC_RED "[%d] " CC_RESET CC_FAINT CC_ITALIC  
                "Error acquiring core lock on exit." CC_RESET, getpid());

        exit(1); 
    }

    if (_core_state->root) {
        // Now, we are going to iterate over the child processes
        // and try to kill each one.
        
        uint64_t i;
        for (i = 0; i < _core_state->children->len; i++) {
            pid_t child = cl_get(_core_state->children, i);
            
            // This should never error if core is used correctly...
            int err = kill(child, SIGKILL);

            if (!err) {
                err = (waitpid(child, NULL, 0) == -1);
            }

            // Log if wanted.
            if (!q) {
                // Check anyway.
                if (err) {
                    core_logf(0, "There was an error terminating process %d.", child);
                } else {
                    core_logf(0, "Process %d was successfully terminated.", child);
                }
            }
        }

        // Delete children.
        delete_child_list(_core_state->children);
    }

    // Only log memory leaks if non quiet.
    if (!q) {
        uint8_t chnl;
        for (chnl = 0; chnl < _core_state->num_mem_chnls; chnl++) {
            if (_core_state->mem_chnls[chnl]) {
                core_logf(0, "Memory leak found in channel %u. (%" PRIu64 ")", 
                        chnl, _core_state->mem_chnls[chnl]);
            }
        }
    }

    // NOTE logging in here using printf could cause major problems!!!
    // We must always make sure our SIGINT handler is blocked when 
    // using printf!

    // Delete the memory channels.
    free(_core_state->mem_chnls);

    // I would free the entire core state here, but unsure what
    // that would do given the core state holds a lock which
    // other threads may be testing at this point.
    //
    // Regardless, we are going to hold onto the core state lock
    // through exit!
    
    // Always exit!
    exit(code);
}

pid_t safe_waitpid(pid_t pid, int *stat_loc, int opts) {
    // We can only use wait if we are a root process!
    _rdlock_core_state();
    if (!(_core_state->root)) {
        return -1;
    }
    _unlock_core_state();

    pid_t res;

    while ((res = waitpid(pid, stat_loc, opts)) == -1) {
        // In case of a hard error, we return -1.
        // Otherwise try again.
        if (errno != EINTR) {
            return -1;
        }
    }

    // Here we successfully reaped a child process.
    // We must remove it from the core children list.
    if (res > 0) {
        _wrlock_core_state();
        uint64_t i;
        for (i = 0; i < _core_state->children->len; i++) {
            if (cl_get(_core_state->children, i) == res) {
                break;
            } 
        }

        if (i == _core_state->children->len) {
            core_logf(0, "Unknown child process was reaped %d.", res);
        } else {
            cl_remove(_core_state->children, i);
        }

        _unlock_core_state();
    }

    return res;
}

#define CORE_RETRY_DELAY_MS 5

pid_t safe_waitpid_timed(pid_t pid, int *stat_loc, time_t timeout) {
    time_t start = time(NULL);

    pid_t res; 
    while (1) {
        res = safe_waitpid(pid, stat_loc, WNOHANG);

        // Successful wait.
        if (res > 0) {
            return res;
        }
        
        // Hard error from waitpid.
        if (res == -1) {
            return -1;
        }

        if (time(NULL) - start >= timeout) {
            return -2;
        }

        usleep(1000 * CORE_RETRY_DELAY_MS);
    }

    // NOTE, should never make it here!
}

int safe_kill_and_reap(pid_t pid) {
    // NOTE, even if the pid identifies a zombie
    // process, this will not error!
    if (kill(pid, SIGKILL)) {
        return -1;
    }

    // If kill succeeds, this should work always.
    // Test for errors just to be safe.
    if (safe_waitpid(pid, NULL, 0) == -1) {
        return -1;
    }

    return 0;
}

