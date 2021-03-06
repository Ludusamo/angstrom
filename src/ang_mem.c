#include "ang_mem.h"

#include "error.h"
#include "ang_primitives.h"
#include "lambda.h"
#include <stdio.h>

void ctor_memory(Memory *mem, size_t gmem_size) {
    mem->gmem = calloc(sizeof(Value*), gmem_size);
    mem->gmem_size = gmem_size;
    mem->global_size = 0;
    mem->num_objects = 0;
    mem->max_objects = 50; // Arbitrary currently
    mem->mem_head = 0;
    for (int i = 0; i < NUM_REGISTERS; i++) {
        mem->registers[i] = 0;
    }
    mem->ip = 0;
    mem->sp = 0;
    mem->fp = 0;
}

void dtor_memory(Memory *mem) {
    // Set stack pointer and gmem_size to 0 so all objects get collected
    mem->sp = 0;
    mem->global_size = 0;
    for (int i = 0; i < NUM_REGISTERS; i++) mem->registers[i] = 0;
    gc(mem);
    free(mem->gmem);
    mem->gmem = 0;
}

Ang_Obj *new_object(Memory *mem, Ang_Type *type) {
    if (mem->num_objects >= mem->max_objects) gc(mem);
    Ang_Obj *obj = calloc(sizeof(Ang_Obj), 1);
    obj->type = type;
    obj->marked = 0;

    obj->next = mem->mem_head;
    mem->mem_head = obj;
    mem->num_objects++;
    return obj;
}

void mark_all_objects(Memory *mem) {
    for (int i = 0; i < mem->sp; i++) {
        mark_ang_obj(mem->stack[i]);
    }
    for (size_t i = 0; i < mem->global_size; i++) {
        mark_ang_obj(mem->gmem[i]);
    }
    for (int i = 0; i < NUM_REGISTERS; i++) {
        mark_ang_obj(mem->registers[i]);
    }
}

void sweep_mem(Memory *mem) {
    Ang_Obj **obj = &mem->mem_head;
    while (*obj) {
        if (!(*obj)->marked) {
            if ((*obj)->type->cat != PRIMITIVE &&
                    (*obj)->type->cat != LAMBDA) { // Is a user defined type
                List *tuple_val = get_ptr((*obj)->v);
                dtor_list(tuple_val);
                free(tuple_val);
                tuple_val = 0;
            } else if ((*obj)->type->cat == LAMBDA) {
                Lambda *l = get_ptr((*obj)->v);
                destroy_lambda(l);
                free(l);
            }
            Ang_Obj *unreachable = *obj;
            *obj = unreachable->next;
            free(unreachable);
            mem->num_objects--;
        } else {
            (*obj)->marked = 0;
            obj = &(*obj)->next;
        }
    }
}

void gc(Memory *mem) {
    mark_all_objects(mem);
    sweep_mem(mem);
    mem->max_objects = mem->num_objects * 2;
}

int push_stack(Memory *mem, Ang_Obj *obj) {
    if (mem->sp >= MAX_STACK_SIZE) {
        runtime_error(STACK_OVERFLOW, "Stack overflow");
        return 0;
    }
    mem->stack[mem->sp++] = obj;
    return 1;
}

Ang_Obj *pop_stack(Memory *mem) {
    if (mem->sp <= 0) {
        runtime_error(STACK_UNDERFLOW, "Stack underflow");
        return 0;
    }
    return mem->stack[--mem->sp];
}

int32_t pop_int(Memory *mem) {
    Ang_Obj *obj = pop_stack(mem);
    return (is_double(obj->v)
        ? (int32_t) obj->v.as_double
        : obj->v.as_int32);
}

double pop_double(Memory *mem) {
    Ang_Obj *obj = pop_stack(mem);
    return (is_double(obj->v)
        ? obj->v.as_double
        : (double) obj->v.as_int32);
}
