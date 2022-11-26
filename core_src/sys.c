#include "./sys.h"
#include "data.h"
#include <pthread.h>
#include <stdlib.h>
#include <sys/_types/_pid_t.h>
#include <sys/_types/_sigset_t.h>
#include <sys/_types/_ssize_t.h>
#include <sys/errno.h>
#include <inttypes.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

// Below lies a list type which doesn't use any of the core safe
// mechanisms, it is specifically for holding pids and that's it!

#define INITIAL_CHILD_LIST_CAP 1

typedef struct child_list_struct {
    uint64_t len;
    uint64_t cap;
    
    pid_t *buf;
} child_list;

child_list *new_child_list() {
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
int cl_add(child_list *cl, pid_t child) {
    // Attempt to realloc if needed.
    if (cl->len == cl->cap) {
        uint64_t new_cap = cl->cap * 2;
        cl->buf = realloc(cl->buf, new_cap);

        // Realloc error.
        if (!(cl->buf)) {
            return -1;
        }

        cl->cap = new_cap;
    }

    cl->buf[cl->len++] = child;
    return 0;
}

void cl_remove(child_list *cl, uint64_t i) {
    if (i >= cl->len) {
        return;
    }

    uint64_t j; // Shift to the left here.
    for (j = i; j < cl->len - 1; j++) {
        cl->buf[j] = cl->buf[j + 1];
    }     

    cl->len--;
}

// When we receive a SIGINT, just redirect to safe_exit.
static void safe_sigint_handler(int signo) {
    (void)signo;

    safe_exit(0);
}

core_state *_core_state = NULL;

// Give us the number of mem chanels to use.
void init_core_state(uint8_t nmcs) {
    // We will not be using safe malloc until we have the core setup.
    if (!(_core_state = malloc(sizeof(core_state)))) {
        core_log("Unable to allocate core state.");
        exit(1);
    }

    _core_state->root = 1;
    pthread_rwlock_init(&(_core_state->core_lock), NULL);

    if (!(_core_state->children = new_child_list())) {
        core_log("Unable to allocate child list.");
        exit(1);
    }

    // Create mem channels array.
    _core_state->num_mem_chnls = nmcs;
    if (!(_core_state->mem_chnls = malloc(nmcs * sizeof(uint64_t)))) {
        core_log("Unable to allocate memory channels.");
        exit(1);
    }

    // Finally, install the standard safe sig int handler.
    signal(SIGINT, safe_sigint_handler); 
}

static void edit_sigint(int how) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);

    pthread_sigmask(how, &set, NULL);
}

void _rdlock_core_state() {
    edit_sigint(SIG_BLOCK);
    pthread_rwlock_rdlock(&(_core_state->core_lock));
}

void _wrlock_core_state() {
    edit_sigint(SIG_BLOCK);
    pthread_rwlock_wrlock(&(_core_state->core_lock));
}

void _unlock_core_state() {
    // NOTE : The order is essential HERE!
    pthread_rwlock_unlock(&(_core_state->core_lock));
    edit_sigint(SIG_UNBLOCK);
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
        core_logf("Child process %d was unable to be recorded. Confirm its termination.", 
                fres); 
    }

    _unlock_core_state();
    return fres;
}

void safe_exit(int code) {
    _wrlock_core_state();

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

            // Check anyway.
            if (err) {
                core_logf("There was an error terminating process %d.", child);
            } else {
                core_logf("Process %d was successfully terminated.", child);
            }
        }

        // Delete children.
        delete_child_list(_core_state->children);
    }

    uint8_t chnl;
    for (chnl = 0; chnl < _core_state->num_mem_chnls; chnl++) {
        if (_core_state->mem_chnls[chnl]) {
            core_logf("Memory leak found in channel %u. (%" PRIu64 " leaks)", 
                    chnl, _core_state->mem_chnls[chnl]);
        }
    }

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
            core_logf("Unknown child process was reaped %d.", res);
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

