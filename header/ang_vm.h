#ifndef ANG_VM_H
#define ANG_VM_H

#include <ang_obj.h>
#include <stdlib.h>

#define MAX_STACK_SIZE 256

typedef enum {
    A, B, C, D, IP, SP, FP, NUM_REGISTERS
} Registers;

typedef struct {
    Ang_Obj *stack[MAX_STACK_SIZE];

    size_t gmem_size;
    size_t global_size;
    Ang_Obj **gmem;

    size_t num_objects;
    size_t max_objects;
    Ang_Obj *mem_head;
    int registers[NUM_REGISTERS];
} Memory;

void ctor_memory(Memory *mem, size_t gmem_size);
void dtor_memory(Memory *mem);

typedef struct {
    Memory mem;
    int running;
    int trace;
    const int *prog;
} Ang_VM;

void ctor_ang_vm(Ang_VM *vm, size_t gmem_size, const int *prog);
void dtor_ang_vm(Ang_VM *vm);

Ang_Obj *new_object(Memory *mem, int type);
void mark_all_objects(Memory *mem);
void sweep_mem(Memory *mem);
void gc(Memory *mem);

int push_stack(Memory *mem, Ang_Obj *obj);
Ang_Obj *pop_stack(Memory *mem);

void eval(Ang_VM *vm, int op);
int fetch(Ang_VM *vm);

void runProgram(const int *program, int main_index, size_t gmem_size);

#endif // ANG_VM_H
