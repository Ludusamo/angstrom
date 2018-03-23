#include "ang_type.h"

#include "ang_primitives.h"

void ctor_ang_type(Ang_Type *type, int id, const char *name, Value default_val) {
    *type = (Ang_Type) { id, name, default_val };
    ctor_hashtable(&type->slots);
    ctor_list(&type->slot_types);
}

void dtor_ang_type(Ang_Type *type) {
    dtor_hashtable(&type->slots);
    dtor_list(&type->slot_types);
}

void add_slot(Ang_Type *type, const char *sym, const Ang_Type *slot_type) {
    int slot_num = type->slots.size;
    set_hashtable(&type->slots, sym, from_double(slot_num));
    append_list(&type->slot_types, from_ptr((void *) slot_type));
}
