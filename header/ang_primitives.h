#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include "ang_type.h"

#define PRIMITIVES(code) \
    code(UNDECLARED) \
    code(NUM_TYPE) \
    code(BOOL_TYPE) \
    code(STRING_TYPE) \
    code(TUPLE_TYPE)

#define DEFINE_ENUM_TYPE(type) type,
typedef enum {
    PRIMITIVES(DEFINE_ENUM_TYPE)
} Primitive_Type;
#undef DEFINE_ENUM_TYPE

typedef struct {
    Value num_default;
    Value bool_default;
    Value string_default;
    Value tuple_default;;
} Primitive_Default;

void ctor_primitive_default(Primitive_Default *defaults);
void dtor_primitive_default(Primitive_Default *defaults);

Ang_Type primitive_ang_type(int id, const Primitive_Default *defaults);

#endif // PRIMITIVES_H
