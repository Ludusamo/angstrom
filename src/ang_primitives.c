#include "ang_primitives.h"

#include <stdlib.h>
#include "list.h"

Ang_Type primitive_ang_type(int id, const Primitive_Default *defaults) {
    switch (id) {
    case NUM_TYPE:
        return (Ang_Type) { id, "Num",  defaults->num_default};
    case BOOL_TYPE:
        return (Ang_Type) { id, "Bool", defaults->bool_default };
    case STRING_TYPE:
        return (Ang_Type) { id, "String", defaults->string_default };
    case TUPLE_TYPE:
        return (Ang_Type) { id, "Tuple", defaults->tuple_default };
    }
    return (Ang_Type) { id, "undeclared", nil_val };
}

void ctor_primitive_default(Primitive_Default *defaults) {
    defaults->num_default = from_double(0);
    defaults->bool_default = false_val;
    defaults->string_default = from_ptr(calloc(0, sizeof(char)));

    List *list = malloc(sizeof(List));
    ctor_list(list);
    defaults->tuple_default = from_ptr(list);
}

void dtor_primitive_default(Primitive_Default *defaults) {
    free(get_ptr(defaults->string_default));
    List *empty_tuple = get_ptr(defaults->tuple_default);
    dtor_list(empty_tuple);
    free(empty_tuple);
    empty_tuple = 0;
}
