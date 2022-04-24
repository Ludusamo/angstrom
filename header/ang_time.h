#ifndef CLOCK_H
#define CLOCK_H

#include "value.h"

#include <time.h>

Value clockNative(Value arg) {
    return from_double((double) clock() / CLOCKS_PER_SEC);
}

#endif // CLOCK_H
