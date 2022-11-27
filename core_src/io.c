#include "./io.h"
#include "sys.h"
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <sys/_types/_sigset_t.h>
#include <unistd.h>
#include <stdarg.h>

void safe_printf(const char *fmt, ...) {
    sigset_t old;

    va_list args; 
    va_start(args, fmt);

    _block_sigint(&old);
    vprintf(fmt, args);
    _restore_sigmask(&old);

    va_end(args);
}

int safe_write(int fd, const void *buf, size_t cnt) {
    ssize_t bytes_left = cnt;
    const char *ptr = buf;

    // NOTE from my understanding printf uses write.
    // Since printf is used in the sigint handler...
    // we must block it here!
    
    sigset_t old;
    _block_sigint(&old);
    while (bytes_left > 0) {
        ssize_t res = write(fd, ptr, bytes_left);

        // Error situation.
        if (res == -1) {
            if (errno != EINTR) {
                _restore_sigmask(&old);
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

    _restore_sigmask(&old);

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
