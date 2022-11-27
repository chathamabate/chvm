#ifndef CORE_IO_H
#define CORE_IO_H

#include <stdlib.h>

// NOTE, printf is used in the core's SIGINT handler
// to display final exit messages.
//
// However, printf (while thread-safe) is not reentrant.
// Thus, when we call printf, we must make sure SIGINT
// is always blocked!
//
// Which is exactly what this call does!
void safe_printf(const char *fmt, ...);

//  Will return -1 on error, 0 on success.
//
//  NOTE : read will return -1 if EOF is found before cnt
//  number of bytes is found!
int safe_write(int fd, const void *buf, size_t cnt);
int safe_read(int fd, void *buf, size_t cnt);
int safe_close(int fd);

#endif
