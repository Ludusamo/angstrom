#include "ang_debug.h"

int num_digits(int num) {
    int count = 1;
    while (num >= 10) {
        num /= 10;
        count++;
    }
    return count;
}

void print_stack_trace(const Ang_VM *vm) {
    int op = fetch(vm);
    int ip = vm->mem.registers[IP];
    int sp = vm->mem.registers[SP];
    int lenOffset = 20 - strlen(opcode_to_str(op)) - num_ops(op); // Length necessary to offset stack print.
    fprintf(stderr, "%04d: %s ", ip, opcode_to_str(op));
    for (int i = 0; i < num_ops(op); i++) {
        fprintf(stderr, "%d ", access_list(&vm->prog, ip + i + 1).as_int32);
        lenOffset -= num_digits(access_list(&vm->prog, ip + i + 1).as_int32);
    }
    fprintf(stderr, "%*s", lenOffset, "[ ");
    for (int i = 0; i < sp; i++) {
        if (is_ptr(vm->mem.stack[i]->v))
            fprintf(stderr, "%lu ", vm->mem.stack[i]->v.bits);
        else if (is_double(vm->mem.stack[i]->v))
            fprintf(stderr, "%.2lf ", vm->mem.stack[i]->v.as_double);
        else
            fprintf(stderr, "%d ", vm->mem.stack[i]->v.as_int32);
    }
    fprintf(stderr, "]\n");
}
