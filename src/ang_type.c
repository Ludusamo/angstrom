#include "ang_type.h"

void ctor_ang_type(Ang_Type *type, int id, const char *name, Value default_val) {
    *type = (Ang_Type) { id, name, default_val };
    ctor_hashtable(&type->slots);
}

void dtor_ang_type(Ang_Type *type) {
    dtor_hashtable(&type->slots);
}
