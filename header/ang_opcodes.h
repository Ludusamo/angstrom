#ifndef ANG_OPCODES_H
#define ANG_OPCODES_H

#define OPCODES(code) \
    code(HALT, 0) \
    code(PUSH, 1) \
    code(PUSOBJ, 2) \
    code(PUSH_0, 0) \
    code(POP, 0) \
    code(POPN, 1) \
    code(ADD, 0) \
    code(SUB, 0) \
    code(MUL, 0) \
    code(DIV, 0) \
    code(ADDF, 0) \
    code(SUBF, 0) \
    code(MULF, 0) \
    code(DIVF, 0) \
    code(LTZ, 0) \
    code(GTZ, 0) \
    code(EQ, 0) \
    code(NEG, 0) \
    code(CMP_TYPE, 1) \
    code(CMP_STRUCT, 1) \
    code(JE, 1) \
    code(JNE, 1) \
    code(GSTORE, 1) \
    code(GLOAD, 1) \
    code(STORE, 1) \
    code(LOAD, 1) \
    code(STORET, 0) \
    code(PUSRET, 0) \
    code(CONS_TUPLE, 2) \
    code(LOAD_TUPLE, 0) \
    code(SET_TUPLE, 1) \
    code(CONS_ARR, 2) \
    code(ACCESS_ARR, 0) \
    code(SET_ARR, 0) \
    code(CONS_LAMBDA, 2) \
    code(SET_FP, 0) \
    code(RESET_FP, 0) \
    code(DUP, 0) \
    code(SET_DEFAULT_VAL, 1) \
    code(LOAD_DEFAULT_VAL, 1) \
    code(STO_REG, 1) \
    code(LOAD_REG, 1) \
    code(SWAP_REG, 2) \
    code(MOV_REG, 2) \
    code(JMP, 1) \
    code(CALL, 0) \
    code(RET, 0)

#define DEFINE_ENUM_TYPE(type, _) type,
typedef enum {
    OPCODES(DEFINE_ENUM_TYPE)
} Opcode;
#undef DEFINE_ENUM_TYPE

const char *opcode_to_str(Opcode op);
int num_ops(Opcode op);

#endif //ANG_OPCODES_H
