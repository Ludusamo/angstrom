#include "ang_vm.h"

#include "error.h"
#include <stdio.h>
#include "ang_opcodes.h"
#include "ang_primitives.h"
#include "ang_debug.h"

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
    // Set stack pointer and gmem_size to 0 so all objects get collected
    mem->registers[SP] = 0;
    mem->gmem_size = 0;
    gc(mem);
    free(mem->gmem);
    mem->gmem = 0;
}

void ctor_ang_vm(Ang_VM *vm, size_t gmem_size) {
    ctor_memory(&vm->mem, gmem_size);
    vm->running = 0;
    vm->trace = 0;
    ctor_list(&vm->prog);
}

void dtor_ang_vm(Ang_VM *vm) {
    dtor_memory(&vm->mem);
    dtor_list(&vm->prog);
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

int push_num_stack(Memory *mem, double d) {
    Ang_Obj *obj = new_object(mem, NUM);
    obj->v = from_double(d);
    push_stack(mem, obj);
    return 1;
}

Ang_Obj *pop_stack(Memory *mem) {
    if (mem->registers[SP] <= 0) {
        runtime_error(STACK_UNDERFLOW, "Stack underflow");
        return 0;
    }
    return mem->stack[--mem->registers[SP]];
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

int get_next_op(Ang_VM *vm) {
    return access_list(&vm->prog, vm->mem.registers[IP]++).as_int32;
}

void eval(Ang_VM *vm) {
    int op = fetch(vm);
    if (vm->trace) print_stack_trace(vm);
    vm->mem.registers[IP]++;
    switch (op) {
    case HALT:
        vm->running = 0;
        break;
    case PUSH:
        push_num_stack(&vm->mem, get_next_op(vm));
        break;
    case POP:
        pop_stack(&vm->mem);
        break;
    case ADD:
        push_num_stack(&vm->mem, pop_int(&vm->mem) + pop_int(&vm->mem));
        break;
    case SUB: {
        int right = pop_int(&vm->mem);
        int left = pop_int(&vm->mem);
        push_num_stack(&vm->mem, left - right);
        break;
    }
    case MUL:
        push_num_stack(&vm->mem, pop_int(&vm->mem) * pop_int(&vm->mem));
        break;
    case DIV: {
        int right = pop_int(&vm->mem);
        int left = pop_int(&vm->mem);
        push_num_stack(&vm->mem, left / right);
        break;
    }
    case ADDF:
        push_num_stack(&vm->mem, pop_double(&vm->mem) + pop_double(&vm->mem));
        break;
    case SUBF: {
        double right = pop_double(&vm->mem);
        double left = pop_double(&vm->mem);
        push_num_stack(&vm->mem, left - right);
        break;
    }
    case MULF:
        push_num_stack(&vm->mem, pop_double(&vm->mem) * pop_double(&vm->mem));
        break;
    case DIVF: {
        double right = pop_double(&vm->mem);
        double left = pop_double(&vm->mem);
        push_num_stack(&vm->mem, left / right);
        break;
    }
    }
}

int fetch(const Ang_VM *vm) {
    return access_list(&vm->prog, vm->mem.registers[IP]).as_int32;
}

int emit_op(Ang_VM *vm, int op) {
    append_list(&vm->prog, from_double(op));
    return vm->prog.length - 1;
}
