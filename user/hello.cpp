#include <stdio.h>
#include <stdlib.h>

void __attribute__((constructor)) before_main() {
    printf("[constructor] Initializing libc environment dynamically via .init_array! \n");
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("======================================\n");
    printf("  Hello from HELLO.ELF (Dynamically linked)!\n");
    printf("  This app uses libc.so loaded by ld.so\n");
    printf("======================================\n");

    printf("\nTesting malloc...\n");
    int* array = (int*)malloc(5 * sizeof(int));
    if (array) {
        printf("Malloc returned: %p\n", (void*)array);
        for (int i = 0; i < 5; i++) {
            array[i] = i * 10;
        }
        for (int i = 0; i < 5; i++) {
            printf("array[%d] = %d\n", i, array[i]);
        }
        printf("Freeing array...\n");
        free(array);
        printf("Free successful.\n");
    } else {
        printf("Malloc failed!\n");
    }

    printf("\nTesting exit(42)...\n");
    exit(42);
    
    // Should never reach here
    printf("ERROR: exit() returned!\n");
    return 0;
}
