#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include "ang_type.h"

#define PRIMITIVES(code) \
    code(UNDECLARED) \
    code(ANY_TYPE) \
    code(NUM_TYPE) \
    code(BOOL_TYPE) \
    code(STRING_TYPE) \
    code(PRIMITIVE_COUNT)

#define DEFINE_ENUM_TYPE(type) type,
typedef enum {
    PRIMITIVES(DEFINE_ENUM_TYPE)
} Primitive_Type;
#undef DEFINE_ENUM_TYPE

typedef struct {
    Ang_Type und_default;
    Ang_Type any_default;
    Ang_Type num_default;
    Ang_Type bool_default;
    Ang_Type string_default;
    Ang_Type tuple_default;
} Primitive_Types;

void ctor_primitive_types(Primitive_Types *defaults);
void dtor_primitive_types(Primitive_Types *defaults);

#endif // PRIMITIVES_H
