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
    code(GSTORE, 1) \
    code(GLOAD, 1) \
    code(STORE, 1) \
    code(LOAD, 1) \
    code(STORET, 0) \
    code(PUSRET, 0)

#define DEFINE_ENUM_TYPE(type, _) type,
typedef enum {
    OPCODES(DEFINE_ENUM_TYPE)
} Opcode;
#undef DEFINE_ENUM_TYPE

const char *opcode_to_str(Opcode op);
int num_ops(Opcode op);

#endif //ANG_OPCODES_H
