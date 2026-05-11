#pragma once

#include <sys/types.h>

#define O_RDONLY    0x00
#define O_WRONLY    0x01
#define O_RDWR      0x02
#define O_CREAT     0x0040
#define O_EXCL      0x0080
#define O_TRUNC     0x0200
#define O_APPEND    0x0400
#define O_CLOEXEC   0x0800

#ifdef __cplusplus
extern "C" {
#endif

int open(const char* path, int flags, ...);

#ifdef __cplusplus
}
#endif
