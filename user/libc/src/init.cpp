#include <stdlib.h>

extern "C" {
    extern void (*__init_array_start[])(void) __attribute__((weak));
    extern void (*__init_array_end[])(void) __attribute__((weak));

    char** environ = nullptr;
    char* __progname = nullptr;

    void __libc_csu_init(void) {
        if (!__init_array_start || !__init_array_end) return;
        size_t count = __init_array_end - __init_array_start;
        for (size_t i = 0; i < count; i++) {
            if (__init_array_start[i]) {
                __init_array_start[i]();
            }
        }
    }

    int __libc_start_main(
        int (*main)(int, char**, char**),
        int argc,
        char** argv,
        void (*init)(void),
        void (*fini)(void),
        void (*rtld_fini)(void),
        void* stack_end
    ) {
        if (argv && argv[0]) {
            __progname = argv[0];
        } else {
            __progname = (char*)"";
        }

        // According to conventional ABI, envp directly follows argv array (which is NULL-terminated)
        environ = &argv[argc + 1];

        if (init) init();
        __libc_csu_init();

        int result = main(argc, argv, environ);

        exit(result);
        return result;
    }
}
