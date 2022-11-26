#include "./io.h"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

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
