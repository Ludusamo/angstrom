#include "ang_type.h"

#include "ang_primitives.h"
#include <stdio.h>

void ctor_ang_type(Ang_Type *type,
        int id,
        const char *name,
        Type_Category cat,
        Value default_val) {
    *type = (Ang_Type) { id, name, cat, default_val, 0, 0, 0 };
}

void dtor_ang_type(Ang_Type *type) {
    type->slots = 0;
    type->slot_types = 0;
}

Ang_Type *copy_ang_type(const Ang_Type *src, Ang_Type *dest) {
    ctor_ang_type(dest, src->id, src->name, src->default_value);

    dest->slots = calloc(1, sizeof(Hashtable));
    ctor_hashtable(dest->slots);
    copy_hashtable(src->slots, dest->slots);

    dest->slot_types = calloc(1, sizeof(List));
    ctor_list(dest->slot_types);
    copy_list(src->slot_types, dest->slot_types);

    return dest;
}

int type_equality(const Ang_Type *t1, const Ang_Type *t2) {
    if (t1->id == t2->id) return 1;
    if (t1->cat == PRIMITIVE) return 0;
    if (t1->cat == SUM) {
        if (t2->cat == SUM) {
            for (size_t i = 0; i < t2->slot_types->length; i++) {
                if (!type_equality(t1, get_ptr(access_list(t2->slot_types, i))))
                    return 0;
            }
            return 1;
        }
        for (size_t i = 0; i < t1->slot_types->length; i++) {
            if (type_equality(get_ptr(access_list(t1->slot_types, i)), t2))
                return 1;
        }
        return 0;
    }
    if (t1->cat == PRODUCT) {
        Iter iter;
        iter_hashtable(&iter, t1->slots);
        foreach(iter) {
            // Check to see if they have the same keys
            const Keyval *slot = get_ptr(val_iter_hashtable(&iter));
            Value t2_slot_num = access_hashtable(t2->slots, slot->key);
            if (t2_slot_num.bits == nil_val.bits) {
                destroy_iter_hashtable(&iter);
                return 0;
            }

            // Check to see if the type of their slot is the same
            const Ang_Type *slot_type1 = get_ptr(access_list(t1->slot_types, slot->val.as_int32));
            const Ang_Type *slot_type2 = get_ptr(access_list(t2->slot_types, t2_slot_num.as_int32));
            if (!type_equality(slot_type1, slot_type2)) {
                destroy_iter_hashtable(&iter);
                return 0;
            }
        }
        destroy_iter_hashtable(&iter);
        return 1;
    }
    return 0;
}

void add_slot(Ang_Type *type, const char *sym, const Ang_Type *slot_type) {
    int slot_num = type->slots->size;
    set_hashtable(type->slots, sym, from_double(slot_num));
    append_list(type->slot_types, from_ptr((void *) slot_type));
}
