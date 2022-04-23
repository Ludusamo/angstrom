#include "ang_mem.h"

#include "error.h"
#include "ang_primitives.h"
#include "ang_lambda.h"
#include <stdio.h>

void ctor_memory(Memory *mem, size_t gmem_size) {
    mem->gmem = calloc(sizeof(Value*), gmem_size);
    mem->gmem_size = gmem_size;
    mem->global_size = 0;
    mem->num_objects = 0;
    mem->max_objects = 50; // Arbitrary currently
    mem->mem_head = 0;
    for (int i = 0; i < NUM_REGISTERS; i++) {
        mem->registers[i] = nil_val;
    }
    mem->ip = 0;
    mem->sp = 0;
    mem->fp = 0;
}

void dtor_memory(Memory *mem) {
    // Set stack pointer and gmem_size to 0 so all objects get collected
    mem->sp = 0;
    mem->global_size = 0;
    for (int i = 0; i < NUM_REGISTERS; i++) {
        mem->registers[i] = nil_val;
    }
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
    // Primitives are not wrapped in an Angstrom Object on the stack so we
    // check if they are ptr values.
    for (int i = 0; i < mem->sp; i++) {
        if (is_ptr(mem->stack[i])) {
            printf("Stack: %p\n", get_ptr(mem->stack[i]));
            mark_ang_obj(get_ptr(mem->stack[i]));
        }
    }
    for (size_t i = 0; i < mem->global_size; i++) {
        if (is_ptr(mem->gmem[i])) {
            printf("GMEM: %p\n", get_ptr(mem->gmem[i]));
            mark_ang_obj(get_ptr(mem->gmem[i]));
        }
    }
    for (int i = 0; i < NUM_REGISTERS; i++) {
        if (is_ptr(mem->registers[i])) {
            printf("Reg: %p\n", get_ptr(mem->registers[i]));
            mark_ang_obj(get_ptr(mem->registers[i]));
        }
    }
}

void sweep_mem(Memory *mem) {
    Ang_Obj **obj = &mem->mem_head;
    while (*obj) {
        if (!(*obj)->marked) {
            if ((*obj)->type->id == STRING_TYPE) {
                free(get_ptr((*obj)->v));
            } else if ((*obj)->type->cat != LAMBDA) { // Is a user defined type
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

int push_stack(Memory *mem, Value val) {
    if (mem->sp >= MAX_STACK_SIZE) {
        runtime_error(STACK_OVERFLOW, "Stack overflow");
        return 0;
    }
    mem->stack[mem->sp++] = val;
    return 1;
}

Value pop_stack(Memory *mem) {
    if (mem->sp <= 0) {
        runtime_error(STACK_UNDERFLOW, "Stack underflow");
        return nil_val;
    }
    return mem->stack[--mem->sp];
}

int32_t pop_int(Memory *mem) {
    Value v = pop_stack(mem);
    return (is_double(v)
        ? (int32_t) v.as_double
        : v.as_int32);
}

double pop_double(Memory *mem) {
    Value v = pop_stack(mem);
    return (is_double(v)
        ? v.as_double
        : (double) v.as_int32);
}
