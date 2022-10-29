#include "./sys.h"
#include <sys/_types/_ssize_t.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <errno.h>

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

int safe_write(int fd, const void *buf, size_t cnt) {
    ssize_t bytes_left = cnt;

    while (bytes_left > 0) {
        ssize_t res = write(fd, buf, bytes_left);

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
    }

    // Success!
    return 0;
}

int safe_read(int fd, void *buf, size_t cnt) {
    ssize_t bytes_left = cnt; 
    while (bytes_left > 0) {
        ssize_t res = read(fd, buf, bytes_left);

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

