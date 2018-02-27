#include "ang_debug.h"
#include "utility.h"

void print_stack_trace(const Ang_VM *vm) {
    int op = fetch(vm);
    int ip = vm->mem.ip;
    int sp = vm->mem.sp;
    int lenOffset = 40 - strlen(opcode_to_str(op)) - num_ops(op); // Length necessary to offset stack print.
    fprintf(stderr, "%04d: %s ", ip, opcode_to_str(op));
    for (int i = 0; i < num_ops(op); i++) {
        fprintf(stderr, "%d ", access_list(&vm->prog, ip + i + 1).as_int32);
        lenOffset -= num_digits(access_list(&vm->prog, ip + i + 1).as_int32);
    }
    fprintf(stderr, "%*s", lenOffset, "[ ");
    for (int i = 0; i < sp; i++) {
        if (is_double(vm->mem.stack[i]->v))
            fprintf(stderr, "%.2lf ", vm->mem.stack[i]->v.as_double);
        else if (is_ptr(vm->mem.stack[i]->v))
            fprintf(stderr, "<%s> ", vm->mem.stack[i]->type->name);
        else
            fprintf(stderr, "%d ", vm->mem.stack[i]->v.as_int32);
    }
    fprintf(stderr, "]\n");
}
