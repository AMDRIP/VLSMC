#include "stdlib.h"
#include "sys/syscall.h"

static unsigned long int next_rand = 1;

void exit(int status) {
    syscall(SYS_EXIT, status);
    while (1) {}
}

void abort(void) {
    exit(EXIT_FAILURE);
}

int atoi(const char* str) {
    int res = 0;
    int sign = 1;

    while (*str == ' ' || *str == '\t' || *str == '\n') str++;

    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }

    return res * sign;
}

static void reverse_string(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

char* itoa(int value, char* str, int base) {
    if (base < 2 || base > 36) {
        str[0] = '\0';
        return str;
    }

    int i = 0;
    int isNegative = 0;

    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    if (value < 0 && base == 10) {
        isNegative = 1;
        value = -value;
    }

    unsigned int uvalue = (unsigned int)value;

    while (uvalue != 0) {
        int rem = uvalue % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        uvalue = uvalue / base;
    }

    if (isNegative) {
        str[i++] = '-';
    }

    str[i] = '\0';
    reverse_string(str, i);

    return str;
}

int abs(int j) {
    return (j < 0) ? -j : j;
}

long int labs(long int j) {
    return (j < 0) ? -j : j;
}

div_t div(int numer, int denom) {
    div_t result;
    if (denom == 0) {
        result.quot = 0;
        result.rem = 0;
        return result;
    }
    result.quot = numer / denom;
    result.rem = numer % denom;
    return result;
}

ldiv_t ldiv(long int numer, long int denom) {
    ldiv_t result;
    if (denom == 0) {
        result.quot = 0;
        result.rem = 0;
        return result;
    }
    result.quot = numer / denom;
    result.rem = numer % denom;
    return result;
}

int rand(void) {
    next_rand = next_rand * 1103515245 + 12345;
    return (unsigned int)(next_rand / 65536) % 32768;
}

void srand(unsigned int seed) {
    next_rand = seed;
}

int fork(void) {
    return (int)syscall(SYS_FORK);
}

int execve(const char* path, char* const argv[], char* const envp[]) {
    return (int)syscall(SYS_EXEC, (long)path, (long)argv, (long)envp);
}

int exec(const char* path) {
    char* argv[] = { (char*)path, nullptr };
    char* envp[] = { nullptr };
    return execve(path, argv, envp);
}

int wait(int* status) {
    return (int)syscall(SYS_WAIT, (long)status);
}
