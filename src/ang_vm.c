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

Value get_next_op(Ang_VM *vm) {
    return access_list(&vm->prog, vm->mem.ip++);
}

void eval(Ang_VM *vm) {
    int op = fetch(vm);
    if (vm->trace) print_stack_trace(vm);
    vm->mem.ip++;
    switch (op) {
    case HALT:
        vm->running = 0;
        break;
    case PUSH:
        push_num_stack(vm, get_next_op(vm).as_int32);
        break;
    case PUSOBJ: {
        Ang_Obj *obj = new_object(&vm->mem, get_ptr(get_next_op(vm)));
        obj->v = get_next_op(vm);
        push_stack(&vm->mem, obj);
        break;
    }
    case PUSH_0:
        push_num_stack(vm, 0);
        break;
    case POP:
        pop_stack(&vm->mem);
        break;
    case POPN: {
        int num_local = get_next_op(vm).as_int32;
        for (int i = 0; i < num_local; i++) {
            pop_stack(&vm->mem);
        }
        break;
    }
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
        vm->mem.gmem[get_next_op(vm).as_int32] = pop_stack(&vm->mem);
        break;
    case GLOAD:
        push_stack(&vm->mem, vm->mem.gmem[get_next_op(vm).as_int32]);
        break;
    case STORE:
        vm->mem.stack[vm->mem.fp + get_next_op(vm).as_int32] = pop_stack(&vm->mem);
        break;
    case LOAD:
        push_stack(&vm->mem, vm->mem.stack[vm->mem.fp + get_next_op(vm).as_int32]);
        break;
    case STORET:
        vm->mem.registers[RET_VAL] = pop_stack(&vm->mem);
        break;
    case PUSRET:
        push_stack(&vm->mem, vm->mem.registers[RET_VAL]);
        break;
    }
}

int fetch(const Ang_VM *vm) {
    return access_list(&vm->prog, vm->mem.ip).as_int32;
}

int emit_op(Ang_VM *vm, Value op) {
    append_list(&vm->prog, op);
    return vm->prog.length;
}

void push_num_stack(Ang_VM *vm, double num) {
    Ang_Obj *obj = new_object(&vm->mem, find_type(vm->compiler, "Num"));
    obj->v = from_double(num);
    push_stack(&vm->mem, obj);
}

void run_compiled_instructions(Ang_VM *vm, Compiler *c) {
    vm->compiler = c;
    int last_op = 0;
    for (size_t i = 0; i < c->instr.length; i++) {
        last_op = emit_op(vm, access_list(&c->instr, i));
    }
    while (vm->mem.ip < last_op) eval(vm);
}
