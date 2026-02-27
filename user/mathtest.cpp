#include <stdio.h>
#include <math.h>

int main() {
    printf("=== MATH.H FPU TEST (with %%f) ===\n");

    printf("fabs(-123.456) = %f\n", fabs(-123.456));
    printf("sqrt(2.0)      = %f\n", sqrt(2.0));
    printf("pow(2.0, 8.0)  = %f\n", pow(2.0, 8.0));
    printf("pow(9.0, 0.5)  = %f\n", pow(9.0, 0.5));
    printf("sin(pi/2)      = %f\n", sin(3.14159265 / 2.0));
    printf("cos(0.0)       = %f\n", cos(0.0));

    printf("=== ALL TESTS ISSUED ===\n");
    return 0;
}
