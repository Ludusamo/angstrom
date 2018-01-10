#include "ang_vm.h"

#include "error.h"

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
}

void dtor_memory(Memory *mem) {
    free(mem->gmem);
    mem->gmem = 0;
}

void ctor_ang_vm(Ang_VM *vm, size_t gmem_size, const int *prog) {
    ctor_memory(&vm->mem, gmem_size);
    vm->running = 0;
    vm->trace = 0;
    vm->prog = prog;
}

void dtor_ang_vm(Ang_VM *vm) {
    dtor_memory(&vm->mem);
}

Ang_Obj *new_object(Memory *mem, int type) {
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
    for (int i = 0; i < mem->registers[SP]; i++) {
        mark_ang_obj(mem->stack[i]);
    }
    for (size_t i = 0; i < mem->global_size; i++) {
        mark_ang_obj(mem->gmem[i]);
    }
}

void sweep_mem(Memory *mem) {
    Ang_Obj **obj = &mem->mem_head;
    while (*obj) {
        if (!(*obj)->marked) {
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
    if (mem->registers[SP] >= MAX_STACK_SIZE) {
        runtime_error(STACK_OVERFLOW, "Stack overflow");
        return 0;
    }
    mem->stack[mem->registers[SP]++] = obj;
    return 1;
}

Ang_Obj *pop_stack(Memory *mem) {
    if (mem->registers[SP] <= 0) {
        runtime_error(STACK_UNDERFLOW, "Stack underflow");
        return 0;
    }
    return mem->stack[--mem->registers[SP]];
}
