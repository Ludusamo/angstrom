#ifndef ERROR_H
#define ERROR_H

#define ERROR_LIST(code) \
    code(UNEXPECTED_CHARACTER) \
    code(UNEXPECTED_TOKEN) \
    code(UNTERMINATED_STRING) \

#define DEFINE_ENUM_CODE(type) type,
typedef enum {
    ERROR_LIST(DEFINE_ENUM_CODE)
} Error_Code;
#undef DEFINE_ENUM_TYPE

const char *error_code_to_str(Error_Code c);
void error(int line, Error_Code c, const char *message);

#endif // ERROR_H
