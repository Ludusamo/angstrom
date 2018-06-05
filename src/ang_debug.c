#include "ang_debug.h"
#include "ang_primitives.h"
#include "utility.h"

void print_stack_trace(const Ang_VM *vm) {
    int op = fetch(vm);
    int ip = vm->mem.ip;
    int sp = vm->mem.sp;
    int lenOffset = 40 - strlen(opcode_to_str(op)) - num_ops(op); // Length necessary to offset stack print.
    fprintf(stderr, "%04d: %s ", ip, opcode_to_str(op));
    for (int i = 0; i < num_ops(op); i++) {
        fprintf(stderr, "%d ", access_list(&INSTR(vm), ip + i + 1).as_int32);
        lenOffset -= num_digits(access_list(&INSTR(vm), ip + i + 1).as_int32);
    }
    fprintf(stderr, "%*s", lenOffset, "[ ");
    for (int i = 0; i < sp; i++) {
        Value v = vm->mem.stack[i]->v;
        const Ang_Type *t = vm->mem.stack[i]->type;
        if (t->id == BOOL_TYPE) {
            fprintf(stderr, "%s ", v.bits == true_val.bits
                ? "true"
                : "false");
        } else if (is_double(v))
            fprintf(stderr, "%.2lf ", v.as_double);
        else if (is_ptr(v)) {
            fprintf(stderr, "<%s> ", vm->mem.stack[i]->type->name);
        }
        else
            fprintf(stderr, "%d ", v.as_int32);
    }
    fprintf(stderr, "]\n");
}
