#ifndef ANG_OPCODES_H
#define ANG_OPCODES_H

#define OPCODES(code) \
    code(HALT, 0) \
    code(PUSH, 1) \
    code(POP, 0) \
    code(ADD, 0) \
    code(SUB, 0) \
    code(MUL, 0) \
    code(DIV, 0) \

#define DEFINE_ENUM_TYPE(type, _) type,
typedef enum {
    OPCODES(DEFINE_ENUM_TYPE)
} Opcode;
#undef DEFINE_ENUM_TYPE

const char *opcode_to_str(Opcode op);
int num_ops(Opcode op);

#endif //ANG_OPCODES_H
