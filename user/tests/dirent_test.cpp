#include <stdio.h>
#include <dirent.h>
#include <assert.h>
#include <limits.h>

int main(int argc, char** argv) {
    printf("========================================\n");
    printf("        DIRENT & ASSERTS TEST           \n");
    printf("========================================\n\n");

    const char* target_path = (argc > 1) ? argv[1] : "/";
    printf("Opening directory: %s\n", target_path);

    DIR* dir = opendir(target_path);
    if (!dir) {
        printf("Failed to open directory!\n");
        return 1;
    }

    printf("Compiler INT_MAX: %d\n\n", INT_MAX);

    printf("%-15s %-10s %s\n", "NAME", "SIZE", "TYPE");
    printf("----------------------------------------\n");

    struct dirent* entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        const char* type_str = (entry->d_type == DT_DIR) ? "<DIR>" : "FILE";
        if (entry->d_name[0] == '\0') continue; // Skip empty
        
        printf("%-15s %-10d %s\n", entry->d_name, entry->d_size, type_str);
        count++;
    }

    printf("\nTotal entries: %d\n", count);
    
    int close_res = closedir(dir);
    assert(close_res == 0);

    printf("Closed successfully. Test complete.\n");
    return 0;
}
