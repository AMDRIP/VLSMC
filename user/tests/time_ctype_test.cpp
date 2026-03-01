#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include "../app_api.h"

int main() {
    printf("========================================\n");
    printf("         LIBC TIME & CTYPE TEST         \n");
    printf("========================================\n\n");

    printf("--- Ctype Test ---\n");
    printf("isalpha('A'): %d (expected >0)\n", isalpha('A'));
    printf("isalpha('1'): %d (expected 0)\n", isalpha('1'));
    printf("isdigit('5'): %d (expected >0)\n", isdigit('5'));
    printf("isspace(' '): %d (expected >0)\n", isspace(' '));
    printf("toupper('a'): %c (expected A)\n", toupper('a'));
    printf("tolower('B'): %c (expected b)\n", tolower('B'));
    
    printf("\n--- Time Test ---\n");
    time_t now = time(nullptr);
    printf("Unix Time: %d\n", now);
    
    struct tm* t = localtime(&now);
    if (t) {
        printf("Localtime: %d-%d-%d %d:%d:%d\n",
               t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
               t->tm_hour, t->tm_min, t->tm_sec);
    } else {
        printf("localtime() returned NULL!\n");
    }
    
    printf("System Uptime: %d ms\n", clock());
    printf("\nTest complete.\n");
    
    return 0;
}
