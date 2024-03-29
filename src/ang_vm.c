#include "ang_vm.h"

#include "error.h"
#include <stdio.h>
#include "ang_opcodes.h"
#include "ang_primitives.h"
#include "ang_debug.h"
#include "lambda.h"
#include <math.h>

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
        push_stack(&vm->mem, from_ptr(obj));
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
    case LTZ: {
        double value = pop_double(&vm->mem);
        Value res = value < 0 ? true_val : false_val;
        push_stack(&vm->mem, res);
        break;
    }
    case GTZ: {
        double value = pop_double(&vm->mem);
        Value res = value > 0 ? true_val : false_val;
        push_stack(&vm->mem, res);
        break;
    }
    case EQ: {
        Value res = pop_stack(&vm->mem).bits == pop_stack(&vm->mem).bits
            ? true_val
            : false_val;
        push_stack(&vm->mem, res);
        break;
    }
    case NEG: {
        Value res = pop_stack(&vm->mem).bits == true_val.bits
            ? false_val
            : true_val;
        push_stack(&vm->mem, res);
        break;
    }
#define CMP_CODE(code) \
    {\
    Value obj1 = pop_stack(&vm->mem);\
    const Ang_Type *t1 = 0; \
    if (is_nil(obj1)) t1 = find_type(&vm->compiler, "Null"); \
    else if (is_bool(obj1)) t1 = find_type(&vm->compiler, "Bool"); \
    else if (is_int32(obj1)) t1 = find_type(&vm->compiler, "Num"); \
    else if (is_double(obj1)) t1 = find_type(&vm->compiler, "Num"); \
    else t1 = ((Ang_Obj *) get_ptr(obj1))->type; \
    const Ang_Type *t2 = get_ptr(get_next_op(vm)); \
    Value res = code(t1, t2) \
        ? true_val \
        : false_val; \
    push_stack(&vm->mem, res); }
    case CMP_TYPE:
        CMP_CODE(type_equality)
        break;
    case CMP_STRUCT:
        CMP_CODE(type_structure_equality)
        break;
#undef CMP_CODE
    case JE: {
        int jmp_loc = get_next_op(vm).as_int32;
        vm->mem.ip = pop_stack(&vm->mem).bits == true_val.bits
            ? jmp_loc
            : vm->mem.ip;
        break;
    }
    case JNE: {
        int jmp_loc = get_next_op(vm).as_int32;
        vm->mem.ip = pop_stack(&vm->mem).bits == false_val.bits
            ? jmp_loc
            : vm->mem.ip;
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
            append_list(tuple_vals, nil_val);
        }
        obj->v = from_ptr(tuple_vals);
        push_stack(&vm->mem, from_ptr(obj));
        break;
    }
    case SET_TUPLE: {
        Value v = pop_stack(&vm->mem);
        Ang_Obj *tup = get_ptr(pop_stack(&vm->mem));
        List *vals = get_ptr(tup->v);
        set_list(vals, get_next_op(vm).as_int32, v);
        push_stack(&vm->mem, from_ptr(tup));
        break;
    }
    case LOAD_TUPLE: {
        int slot_num = pop_int(&vm->mem);
        Ang_Obj *tuple = get_ptr(pop_stack(&vm->mem));
        Ang_Obj *obj = get_ptr(access_list(get_ptr(tuple->v), slot_num));
        push_stack(&vm->mem, from_ptr(obj));
        break;
    }
    case CONS_ARR: {
        Ang_Obj *obj = new_object(&vm->mem, get_ptr(get_next_op(vm)));
        int num_ele = get_next_op(vm).as_int32;
        List *l = malloc(sizeof(List));
        ctor_list(l);
        for (int i = 0; i < num_ele; i++) {
            append_list(l, pop_stack(&vm->mem));
        }
        obj->v = from_ptr(l);
        push_stack(&vm->mem, from_ptr(obj));
        break;
    }
    case ACCESS_ARR: {
        Ang_Obj *arr_obj = get_ptr(pop_stack(&vm->mem));
        Value index = pop_stack(&vm->mem);
        List *arr = get_ptr(arr_obj->v);
        if (!is_int32(index) || index.as_int32 >= arr->length) {
            push_stack(&vm->mem, nil_val);
        }
        push_stack(&vm->mem, access_list(arr, index.as_int32));
        break;
    }
    case SET_ARR: {
        Ang_Obj *arr_obj = get_ptr(pop_stack(&vm->mem));
        Value index = pop_stack(&vm->mem);
        Ang_Obj *rhs = get_ptr(pop_stack(&vm->mem));
        List *arr = get_ptr(arr_obj->v);
        if (!is_int32(index) || index.as_int32 >= arr->length) {
            runtime_error(ARR_OUT_OF_BOUNDS,
                "Attempted to assign out of array bounds.\n");
        }
        set_list(arr, index.as_int32, from_ptr(rhs));
        push_stack(&vm->mem, from_ptr(arr_obj));
        break;
    }
    case CONS_LAMBDA: {
        Ang_Obj *obj = new_object(&vm->mem, get_ptr(get_next_op(vm)));
        Lambda *l = malloc(sizeof(Lambda));
        l->ip = get_next_op(vm).as_int32;
        save_lambda_env(l, &vm->mem);
        obj->v = from_ptr(l);
        push_stack(&vm->mem, from_ptr(obj));
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
        find_type(&vm->compiler, type_name)->default_value = pop_stack(&vm->mem);
        break;
    }
    case LOAD_DEFAULT_VAL: {
        Ang_Type *type = get_ptr(get_next_op(vm));
        push_stack(&vm->mem, type->default_value);
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
        Value tmp = vm->mem.registers[reg1];
        vm->mem.registers[reg1] = vm->mem.registers[reg2];
        vm->mem.registers[reg2] = tmp;
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
        vm->mem.registers[A] = pop_stack(&vm->mem);
        Value l_val = ((Ang_Obj *) get_ptr(pop_stack(&vm->mem)))->v;
        if (l_val.bits == nil_val.bits) {
            runtime_error(NON_LAMBDA_CALL, "Attempt to call uninitialized lambda\n");
            return;
        }
        Lambda *l = get_ptr(l_val);
        int ip = l->ip;
        push_num_stack(vm, vm->mem.fp);
        push_num_stack(vm, vm->mem.ip);
        vm->mem.fp = vm->mem.sp;
        load_lambda_env(l, &vm->mem);
        vm->mem.ip = ip;
        break;
    }
    case RET:
        vm->mem.registers[RET_VAL] = pop_stack(&vm->mem);
        vm->mem.sp = vm->mem.fp;
        vm->mem.ip = pop_stack(&vm->mem).as_int32;
        vm->mem.fp = pop_stack(&vm->mem).as_int32;
        push_stack(&vm->mem, vm->mem.registers[RET_VAL]);
        break;
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
    push_stack(&vm->mem, from_double(num));
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
    vm->mem.global_size = vm->compiler.env.symbols.size;
    while (vm->mem.ip < INSTR(vm).length) eval(vm);
}
