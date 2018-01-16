#include "ang_vm.h"

#include "error.h"
#include <stdio.h>
#include "ang_opcodes.h"
#include "ang_primitives.h"
#include "ang_debug.h"

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
        push_num_stack(vm, get_next_op(vm));
        break;
    case PUSH_0:
        push_num_stack(vm, 0);
        break;
    case POP:
        pop_stack(&vm->mem);
        break;
    case ADD:
        push_num_stack(vm, pop_int(&vm->mem) + pop_int(&vm->mem));
        break;
    case SUB: {
        int right = pop_int(&vm->mem);
        int left = pop_int(&vm->mem);
        push_num_stack(vm, left - right);
        break;
    }
    case MUL:
        push_num_stack(vm, pop_int(&vm->mem) * pop_int(&vm->mem));
        break;
    case DIV: {
        int right = pop_int(&vm->mem);
        int left = pop_int(&vm->mem);
        push_num_stack(vm, left / right);
        break;
    }
    case ADDF:
        push_num_stack(vm, pop_double(&vm->mem) + pop_double(&vm->mem));
        break;
    case SUBF: {
        double right = pop_double(&vm->mem);
        double left = pop_double(&vm->mem);
        push_num_stack(vm, left - right);
        break;
    }
    case MULF:
        push_num_stack(vm, pop_double(&vm->mem) * pop_double(&vm->mem));
        break;
    case DIVF: {
        double right = pop_double(&vm->mem);
        double left = pop_double(&vm->mem);
        push_num_stack(vm, left / right);
        break;
    }
    case GSTORE:
        vm->mem.gmem[get_next_op(vm)] = pop_stack(&vm->mem);
        break;
    case GLOAD:
        push_stack(&vm->mem, vm->mem.gmem[get_next_op(vm)]);
        break;
    case STORE:
        break;
    case LOAD:
        push_stack(&vm->mem, vm->mem.stack[vm->mem.registers[FP] + get_next_op(vm)]);
        break;
    }
}

int fetch(const Ang_VM *vm) {
    return access_list(&vm->prog, vm->mem.registers[IP]).as_int32;
}

int emit_op(Ang_VM *vm, int op) {
    append_list(&vm->prog, from_double(op));
    return vm->prog.length - 1;
}

void push_num_stack(Ang_VM *vm, double num) {
    Ang_Obj *obj = new_object(&vm->mem, find_type(vm->compiler, "num"));
    obj->v = from_double(num);
    push_stack(&vm->mem, obj);
}
