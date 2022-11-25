#include "./sys.h"
#include "data.h"
#include <pthread.h>
#include <stdlib.h>
#include <sys/_types/_ssize_t.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

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

void safe_exit(int code, uint8_t check_mem) {
    // These two lines confirm we have the write lock acquired
    // before doing an editing of the _core_state.
    pthread_rwlock_unlock(&(_core_state->core_lock));
    pthread_rwlock_wrlock(&(_core_state->core_lock));

    if (_core_state->root) {
        // Now, we are going to iterate over the child processes
        // and try to kill each one.
        
        uint64_t i;
        for (i = 0; i < _core_state->children->len; i++) {
            pid_t child = *(pid_t *)sl_get(_core_state->children, i);
            
            // This should never error if core is used correctly...
            int res = kill(child, SIGKILL);
            res |= waitpid(child, NULL, 0);

            if (res) {
                // TODO, finish this up soon!
            }
        }
    }
    
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

