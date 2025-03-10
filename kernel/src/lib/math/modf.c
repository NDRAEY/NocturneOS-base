#include "lib/math.h"

double modf(double value, double* intPart) {
    if (intPart != NULL) {
        *intPart = (value >= 0) ? floor(value) : ceil(value);
        return value - (*intPart);
    }
    return 0.0f;
}

double fmod(double dividend, double divisor) {
    if (divisor == 0.0) {
        return NAN;
    }

    double quotient = dividend / divisor;
    int intPart = (int) quotient; // Integer part
    double fractionalPart = quotient - intPart; // Fractional part

    double result = fractionalPart * divisor;
    return result;
}

float fmodf(float x, float y) {
    if (y == 0.0f) {
        // Handle division by zero (return NaN as per standard)
        return NAN;
    }

    // Calculate the quotient as an integer
    float quotient = x / y;
    int intPart = (int) quotient; // Integer part

    // Calculate the remainder
    float remainder = (x - intPart) * y;

    return remainder;
}
