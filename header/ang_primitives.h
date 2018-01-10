#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#define PRIMITIVES(code) \
    code(NUM) \
    code(BOOL) \
    code(STRING)

#define DEFINE_ENUM_TYPE(type) type,
typedef enum {
    PRIMITIVES(DEFINE_ENUM_TYPE)
} Primitive_Type;
#undef DEFINE_ENUM_TYPE

#endif // PRIMITIVES_H
