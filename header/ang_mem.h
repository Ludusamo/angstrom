#ifndef ANG_MEM_H
#define ANG_MEM_H

#include "ang_obj.h"
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

Ang_Obj *new_object(Memory *mem, int type);
void mark_all_objects(Memory *mem);
void sweep_mem(Memory *mem);
void gc(Memory *mem);

int push_stack(Memory *mem, Ang_Obj *obj);
int push_num_stack(Memory *mem, double d);

Ang_Obj *pop_stack(Memory *mem);
int32_t pop_int(Memory *mem);
double pop_double(Memory *mem);

#endif // ANG_MEM_H
