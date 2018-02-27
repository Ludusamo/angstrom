#include "utility.h"

#include <math.h>

int num_digits(int num) {
    if (num == 0) return 1;
    if (num < 0) return (int) log10(-num) + 2;
    return (int) log10(num) + 1;
}
