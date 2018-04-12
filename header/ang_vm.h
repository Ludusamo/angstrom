#ifndef ANG_VM_H
#define ANG_VM_H

#include <stdlib.h>
#include "ang_obj.h"
#include "ang_mem.h"
#include "list.h"
#include "compiler.h"

#define INSTR(vm) vm->compiler.instr

typedef struct {
    Memory mem;
    int running;
    int trace;

    Compiler compiler;
} Ang_VM;

void ctor_ang_vm(Ang_VM *vm, size_t gmem_size);
void dtor_ang_vm(Ang_VM *vm);

Value get_next_op(Ang_VM *vm);
void eval(Ang_VM *vm);
int fetch(const Ang_VM *vm);

int emit_op(Ang_VM *vm, Value op);

void push_num_stack(Ang_VM *vm, double num);

void run_compiled_instructions(Ang_VM *vm, Compiler *c);

void run_code(Ang_VM *vm, const char *code, const char *src_name);

#endif // ANG_VM_H
