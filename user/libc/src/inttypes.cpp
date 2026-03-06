#include <inttypes.h>
#include <stdlib.h>

intmax_t imaxabs(intmax_t j) { return (j < 0) ? -j : j; }

imaxdiv_t imaxdiv(intmax_t numer, intmax_t denom) {
    imaxdiv_t res;
    res.quot = numer / denom;
    res.rem = numer % denom;
    return res;
}

intmax_t strtoimax(const char *nptr, char **endptr, int base) { return strtoll(nptr, endptr, base); }
uintmax_t strtoumax(const char *nptr, char **endptr, int base) { return strtoull(nptr, endptr, base); }
