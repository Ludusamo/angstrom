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
    vm->enc_err = 0;
    vm->compiler.enc_err = &vm->enc_err;
    ctor_compiler(&vm->compiler);
}

void dtor_ang_vm(Ang_VM *vm) {
    dtor_memory(&vm->mem);
    dtor_compiler(&vm->compiler);
}

Value get_next_op(Ang_VM *vm) {
    return access_list(&INSTR(vm), vm->mem.ip++);
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
        push_num_stack(vm, get_next_op(vm).as_double);
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
    case CONS_TUPLE: {
        Ang_Obj *obj = new_object(&vm->mem, get_ptr(get_next_op(vm)));
        List *tuple_vals = malloc(sizeof(List));
        ctor_list(tuple_vals);
        int num_slots = get_next_op(vm).as_int32;
        for (int i = 0; i < num_slots; i++) {
            append_list(tuple_vals, from_ptr(pop_stack(&vm->mem)));
        }
        obj->v = from_ptr(tuple_vals);
        push_stack(&vm->mem, obj);
        break;
    }
    case LOAD_TUPLE: {
        int slot_num = pop_int(&vm->mem);
        Ang_Obj *tuple = pop_stack(&vm->mem);
        Ang_Obj *obj = get_ptr(access_list(get_ptr(tuple->v), slot_num));
        push_stack(&vm->mem, obj);
        break;
    }
    case SET_FP:
        vm->mem.fp = vm->mem.sp;
        break;
    case RESET_FP:
        vm->mem.fp = 0;
        break;
    case DUP:
        push_stack(&vm->mem, vm->mem.stack[vm->mem.sp - 1]);
        break;
    case SET_DEFAULT_VAL: {
        const char *type_name = get_ptr(get_next_op(vm));
        find_type(&vm->compiler, type_name)->default_value = pop_stack(&vm->mem)->v;
        break;
    }
    case STO_REG:
        vm->mem.registers[get_next_op(vm).as_int32] = pop_stack(&vm->mem);
        break;
    case LOAD_REG:
        push_stack(&vm->mem, vm->mem.registers[get_next_op(vm).as_int32]);
        break;
    case SWAP_REG: {
        int reg1 = get_next_op(vm).as_int32;
        int reg2 = get_next_op(vm).as_int32;
        Ang_Obj *temp = vm->mem.registers[reg1];
        vm->mem.registers[reg1] = vm->mem.registers[reg2];
        vm->mem.registers[reg2] = temp;
        break;
    }
    case MOV_REG: {
        int reg1 = get_next_op(vm).as_int32;
        int reg2 = get_next_op(vm).as_int32;
        vm->mem.registers[reg2] = vm->mem.registers[reg1];
        break;
    }
    case JMP:
        vm->mem.ip = get_next_op(vm).as_int32;
        break;
    case CALL: {
        int ip = get_next_op(vm).as_int32;
        int num_args = get_next_op(vm).as_int32;
        push_num_stack(vm, num_args);
        push_num_stack(vm, vm->mem.fp);
        push_num_stack(vm, vm->mem.ip);
        vm->mem.fp = vm->mem.sp;
        vm->mem.ip = ip;
        break;
    }
    case RET:
        vm->mem.registers[RET_VAL] = pop_stack(&vm->mem);
        vm->mem.sp = vm->mem.fp;
        vm->mem.ip = pop_stack(&vm->mem)->v.as_int32;
        vm->mem.fp = pop_stack(&vm->mem)->v.as_int32;
        vm->mem.sp -= pop_stack(&vm->mem)->v.as_int32;
        push_stack(&vm->mem, vm->mem.registers[RET_VAL]);
    }
}

int fetch(const Ang_VM *vm) {
    return access_list(&INSTR(vm), vm->mem.ip).as_int32;
}

int emit_op(Ang_VM *vm, Value op) {
    append_list(&INSTR(vm), op);
    return INSTR(vm).length;
}

void push_num_stack(Ang_VM *vm, double num) {
    Ang_Obj *obj = new_object(&vm->mem, find_type(&vm->compiler, "Num"));
    obj->v = from_double(num);
    push_stack(&vm->mem, obj);
}

void run_compiled_instructions(Ang_VM *vm, Compiler *c) {
    int last_op = 0;
    for (size_t i = 0; i < c->instr.length; i++) {
        last_op = emit_op(vm, access_list(&c->instr, i));
    }
    while (vm->mem.ip < last_op) eval(vm);
}

void run_code(Ang_VM *vm, const char *code, const char *src_name) {
    compile_code(&vm->compiler, code, src_name);
    if (vm->enc_err) return;
    while (vm->mem.ip < INSTR(vm).length) eval(vm);
}
