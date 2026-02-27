#pragma once

#include <stddef.h>
#include <malloc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 32767

typedef struct {
    int quot;
    int rem;
} div_t;

typedef struct {
    long quot;
    long rem;
} ldiv_t;

void exit(int status);
void abort(void);

int atoi(const char* str);
char* itoa(int value, char* str, int base);

int abs(int j);
long int labs(long int j);

div_t div(int numer, int denom);
ldiv_t ldiv(long int numer, long int denom);

int rand(void);
void srand(unsigned int seed);

#ifdef __cplusplus
}
#endif
