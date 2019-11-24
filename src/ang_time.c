#include "ang_time.h"

#include <time.h>

int ang_time(Ang_VM *vm) {
    time_t t;
    time(&t);
    push_num_stack(vm, t);
    return 1;
}
