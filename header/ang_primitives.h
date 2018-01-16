#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#define PRIMITIVES(code) \
    code(UNDECLARED) \
    code(NUM_TYPE) \
    code(BOOL_TYPE) \
    code(STRING_TYPE)

#define DEFINE_ENUM_TYPE(type) type,
typedef enum {
    PRIMITIVES(DEFINE_ENUM_TYPE)
} Primitive_Type;
#undef DEFINE_ENUM_TYPE

#endif // PRIMITIVES_H
