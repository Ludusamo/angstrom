#ifndef ERROR_H
#define ERROR_H

#define ERROR_LIST(code) \
    code(UNEXPECTED_CHARACTER) \
    code(UNEXPECTED_TOKEN) \
    code(UNTERMINATED_STRING) \
    code(STACK_OVERFLOW) \
    code(STACK_UNDERFLOW) \
    code(UNDECLARED_VARIABLE) \
    code(NAME_COLLISION) \
    code(UNKNOWN_TYPE) \
    code(TYPE_ERROR) \
    code(UNCLOSED_BLOCK) \
    code(UNCLOSED_TUPLE)

#define DEFINE_ENUM_CODE(type) type,
typedef enum {
    ERROR_LIST(DEFINE_ENUM_CODE)
} Error_Code;
#undef DEFINE_ENUM_TYPE

const char *error_code_to_str(Error_Code c);
void error(int line, Error_Code c, const char *message);

void runtime_error(Error_Code c, const char *message);

#endif // ERROR_H
