#include "ang_type.h"

#include "ang_primitives.h"
#include <stdio.h>

void ctor_ang_type(Ang_Type *type, int id, const char *name, Value default_val) {
    *type = (Ang_Type) { id, name, default_val };
    type->slots = 0;
    type->slot_types = 0;
}

void dtor_ang_type(Ang_Type *type) {
    type->slots = 0;
    type->slot_types = 0;
}

void add_slot(Ang_Type *type, const char *sym, const Ang_Type *slot_type) {
    int slot_num = type->slots->size;
    set_hashtable(type->slots, sym, from_double(slot_num));
    append_list(type->slot_types, from_ptr((void *) slot_type));
}
