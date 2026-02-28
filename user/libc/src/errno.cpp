#include "errno.h"

extern "C" int* __errno_location(void) {
    // Fallback: static global errno.
    // When kernel supports TLS via sys_set_thread_area + %fs/%gs, we will read from TLS block here.
    static int global_errno = 0;
    return &global_errno;
}
