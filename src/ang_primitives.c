#include "ang_primitives.h"

#include <stdlib.h>
#include "list.h"

void ctor_primitive_types(Primitive_Types *defaults) {
    ctor_ang_type(&defaults->und_default, UNDECLARED, "Und", nil_val);
    ctor_ang_type(&defaults->num_default, NUM_TYPE, "Num", from_double(0));
    ctor_ang_type(&defaults->bool_default, BOOL_TYPE, "Bool", false_val);
    ctor_ang_type(&defaults->string_default,
        STRING_TYPE,
        "String",
        from_ptr(calloc(0, sizeof(char))));
    ctor_ang_type(&defaults->tuple_default, TUPLE_TYPE, "Tuple", nil_val);
    defaults->tuple_default.slots = calloc(1, sizeof(Hashtable));
    ctor_hashtable(defaults->tuple_default.slots);
    defaults->tuple_default.slot_types = calloc(1, sizeof(List));
    ctor_list(defaults->tuple_default.slot_types);
}

void dtor_primitive_types(Primitive_Types *defaults) {
    free(get_ptr(defaults->string_default.default_value));
    dtor_ang_type(&defaults->und_default);
    dtor_ang_type(&defaults->num_default);
    dtor_ang_type(&defaults->bool_default);
    dtor_ang_type(&defaults->string_default);

    dtor_hashtable(defaults->tuple_default.slots);
    free(defaults->tuple_default.slots);
    dtor_list(defaults->tuple_default.slot_types);
    free(defaults->tuple_default.slot_types);
    dtor_ang_type(&defaults->tuple_default);
}
