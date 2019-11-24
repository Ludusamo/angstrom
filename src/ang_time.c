#include "ang_time.h"

#include <time.h>
#include "error.h"
#include "value.h"

int ang_time(Ang_VM *vm) {
    Value arg = vm->mem.registers[A]->v;
    if (arg.bits != nil_val.bits) {
        runtime_error(INVALID_LAMBDA_PARAM, "Call to \"time\" function with non-null.\n");
        return 0;
    }
    time_t t;
    time(&t);
    push_num_stack(vm, t);
    return 1;
}
