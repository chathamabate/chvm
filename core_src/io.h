#ifndef CORE_IO_H
#define CORE_IO_H

#include <stdlib.h>

//  Will return -1 on error, 0 on success.
//
//  NOTE : read will return -1 if EOF is found before cnt
//  number of bytes is found!
int safe_write(int fd, const void *buf, size_t cnt);
int safe_read(int fd, void *buf, size_t cnt);
int safe_close(int fd);

#endif
