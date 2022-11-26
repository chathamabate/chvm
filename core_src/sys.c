#include "./sys.h"
#include "data.h"
#include <pthread.h>
#include <stdlib.h>
#include <sys/_types/_pid_t.h>
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

core_state *_core_state = NULL;

// Give us the number of mem chanels to use.
int init_core_state(uint8_t nmcs) {
    // We will not be using safe malloc until we have the core setup.
    if (!(_core_state = malloc(sizeof(core_state)))) {
        return -1;
    }

    _core_state->root = 1;
    pthread_rwlock_init(&(_core_state->core_lock), NULL);

    if (!(_core_state->children = new_slist_unsafe(sizeof(pid_t)))) {
        free(_core_state);
        _core_state = NULL;

        return -1;
    }

    // Create mem channels array.
    _core_state->num_mem_chnls = nmcs;
    if (!(_core_state->mem_chnls = malloc(nmcs * sizeof(uint64_t)))) {
        delete_slist_unsafe(_core_state->children);
        free(_core_state);
        _core_state = NULL;

        return -1;
    }

    return 0;
}

int safe_fork() {
    // First, lets get the write lock.
    pthread_rwlock_wrlock(&(_core_state->core_lock));

    // For now, we can only fork from the root process.
    if (!(_core_state->root)) {
        pthread_rwlock_unlock(&(_core_state->core_lock));
        return -1;
    }

    pid_t fres = fork();

    // Vanilla fork error.
    if (fres == -1) {
        pthread_rwlock_unlock(&(_core_state->core_lock));
        return -1;
    }

    // Child process.
    if (fres == 0) {
        _core_state->root = 0; 

        // No children list is needed for the child
        // process.
        delete_slist_unsafe(_core_state->children);
        _core_state->children = NULL;
         
        pthread_rwlock_unlock(&(_core_state->core_lock));
        return 0;
    }

    // Parent process. Add the child pid to our children list.
    sl_add(_core_state->children, &fres);

    pthread_rwlock_unlock(&(_core_state->core_lock));
    return fres;
}

void safe_exit(int code) {
    pthread_rwlock_wrlock(&(_core_state->core_lock));

    if (_core_state->root) {
        // Now, we are going to iterate over the child processes
        // and try to kill each one.
        
        uint64_t i;
        for (i = 0; i < _core_state->children->len; i++) {
            pid_t child = *(pid_t *)sl_get(_core_state->children, i);
            
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
        delete_slist_unsafe(_core_state->children);
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
        pthread_rwlock_wrlock(&(_core_state->core_lock));
        uint64_t i;
        for (i = 0; i < _core_state->children->len; i++) {
            if (*(pid_t *)sl_get(_core_state->children, i) == res) {
                break;
            } 
        }

        if (i == _core_state->children->len) {
            core_logf("Unknown child process was reaped %d.", res);
        } else {
            sl_remove(_core_state->children, i);
        }

        pthread_rwlock_unlock(&(_core_state->core_lock));
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

int safe_write(int fd, const void *buf, size_t cnt) {
    ssize_t bytes_left = cnt;
    const char *ptr = buf;

    while (bytes_left > 0) {
        ssize_t res = write(fd, ptr, bytes_left);

        // Error situation.
        if (res == -1) {
            if (errno != EINTR) {
                return -1;
            }

            // If it was simply an interruption,
            // just try again.
            continue;
        }

        // Otherwise, update bytes left.
        bytes_left -= res;
        
        // Advance pointer.
        ptr += res;
    }

    // Success!
    return 0;
}

int safe_read(int fd, void *buf, size_t cnt) {
    ssize_t bytes_left = cnt; 
    char *ptr = buf;

    while (bytes_left > 0) {
        ssize_t res = read(fd, ptr, bytes_left);

        if (res == 0) {
            // EOF found, but not expected.
            return -1;
        }

        if (res == -1) {
            if (errno != EINTR) {
                return -1;
            }

            // Try again after interruption.
            continue;
        }

        // No error... just account for bytes
        // read.
        bytes_left -= res;
        ptr += res;
    }

    return 0;
}

int safe_close(int fd) {
    // close returns 0 on success, -1 on error.
    while (close(fd)) {
        if (errno != EINTR) {
            return -1;
        }
    }

    return 0;
}

