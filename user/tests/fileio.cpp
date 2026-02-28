#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define TEST_SIZE 10000

int main() {
    printf("--- Buffered I/O Test ---\n");
    
    printf("Writing %d bytes (fputc)...\n", TEST_SIZE);
    FILE* fw = fopen("TESTIO  TXT", "w");
    if (!fw) {
        printf("Failed to open TESTIO  TXT for writing.\n");
        return 1;
    }
    
    for (int i = 0; i < TEST_SIZE; i++) {
        fputc('A' + (i % 26), fw);
    }
    fclose(fw);
    printf("Write test complete.\n");
    
    printf("Reading %d bytes (fgetc)...\n", TEST_SIZE);
    FILE* fr = fopen("TESTIO  TXT", "r");
    if (!fr) {
        printf("Failed to open TESTIO  TXT for reading.\n");
        return 1;
    }
    
    int match_count = 0;
    for (int i = 0; i < TEST_SIZE; i++) {
        int c = fgetc(fr);
        if (c == EOF) {
            printf("Premature EOF at byte %d\n", i);
            break;
        }
        if (c == ('A' + (i % 26))) {
            match_count++;
        }
    }
    fclose(fr);
    printf("Read test complete. Bytes matched: %d / %d\n", match_count, TEST_SIZE);
    
    if (match_count == TEST_SIZE) {
        printf("RESULT: SUCCESS. Buffered I/O is working flawlessly.\n");
    } else {
        printf("RESULT: FAILURE.\n");
    }
    
    return 0;
}
