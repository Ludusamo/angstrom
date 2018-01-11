#ifndef ANG_VM_H
#define ANG_VM_H

#include <stdlib.h>
#include "ang_obj.h"
#include "ang_mem.h"
#include "list.h"

typedef struct {
    Memory mem;
    int running;
    int trace;
    List prog;
} Ang_VM;

void ctor_ang_vm(Ang_VM *vm, size_t gmem_size);
void dtor_ang_vm(Ang_VM *vm);

int get_next_op(Ang_VM *vm);
void eval(Ang_VM *vm);
int fetch(const Ang_VM *vm);

int emit_op(Ang_VM *vm, int instruction);

#endif // ANG_VM_H
