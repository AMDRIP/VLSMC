#include <stdio.h>
#include <math.h>

void print_float_approx(const char* name, double value) {
    int int_part = (int)value;
    int frac_part = (int)((value - int_part) * 1000.0);
    if (frac_part < 0) frac_part = -frac_part;
    printf("%s: %d.%03d\n", name, int_part, frac_part);
}

int main() {
    printf("=== MATH.H FPU TEST SUITE ===\n");

    double test_fabs = fabs(-123.456);
    print_float_approx("fabs(-123.456)", test_fabs);

    double test_sqrt = sqrt(2.0);
    print_float_approx("sqrt(2.0)", test_sqrt);

    double test_pow_int = pow(2.0, 8.0);
    print_float_approx("pow(2.0, 8.0)", test_pow_int);

    double test_pow_frac = pow(9.0, 0.5);
    print_float_approx("pow(9.0, 0.5)", test_pow_frac);

    double test_sin = sin(3.14159 / 2.0);
    print_float_approx("sin(pi/2)", test_sin);

    double test_cos = cos(0.0);
    print_float_approx("cos(0.0)", test_cos);

    printf("=== ALL TESTS ISSUED ===\n");
    return 0;
}
