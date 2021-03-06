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
    ctor_ang_type(dest, src->id, src->name, src->cat, src->default_value);

    dest->slots = calloc(1, sizeof(Hashtable));
    ctor_hashtable(dest->slots);
    copy_hashtable(src->slots, dest->slots);

    dest->slot_types = calloc(1, sizeof(List));
    ctor_list(dest->slot_types);
    copy_list(src->slot_types, dest->slot_types);

    return dest;
}

static int sum_type_equality(const Ang_Type *t1, const Ang_Type *t2) {
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

static int product_type_equality(const Ang_Type *t1, const Ang_Type *t2) {
    if (t1->cat != PRODUCT || t2->cat != PRODUCT) return 0;
    if (t1->slot_types->length > t2->slot_types->length) return 0;
    Iter iter;
    iter_hashtable(&iter, t1->slots);
    foreach(iter) {
        // Check to see if they have the same keys
        const Keyval *slot = get_ptr(val_iter_hashtable(&iter));

        Ang_Type *other = get_ptr(access_list(t2->slot_types, slot->val.as_int32));
        if (other->id == ANY_TYPE) continue;
        if (strcmp(slot->key, "_") == 0) continue;

        Value t2_slot_num = access_hashtable(t2->slots, slot->key);
        if (t2_slot_num.bits == nil_val.bits) {
            destroy_iter_hashtable(&iter);
            return 0;
        }

        if (slot->val.as_int32 != t2_slot_num.as_int32) {
            return 0;
        }
    }
    destroy_iter_hashtable(&iter);

    // Check to see if the type of their slots are the same
    for (size_t i = 0; i < t1->slot_types->length; i++) {
        Ang_Type *t1_type = get_ptr(access_list(t1->slot_types, i));
        Ang_Type *t2_type = get_ptr(access_list(t2->slot_types, i));
        if (!type_equality(t1_type, t2_type)) return 0;
    }
    return 1;
}

int type_equality(const Ang_Type *t1, const Ang_Type *t2) {
    if (t1->id == t2->id) return 1;
    else if (t1->id == ANY_TYPE || t2->id == ANY_TYPE) return 1;
    else if (t1->cat == PRIMITIVE) return 0;
    else if (t1->cat == SUM) return sum_type_equality(t1, t2);
    else if (t1->cat == PRODUCT) return product_type_equality(t1, t2);
    return 0;
}

static int sum_type_structure_equality(const Ang_Type *t1, const Ang_Type *t2) {
    if (t2->cat == SUM) {
        for (size_t i = 0; i < t2->slot_types->length; i++) {
            if (!type_structure_equality(t1, get_ptr(access_list(t2->slot_types, i))))
                return 0;
        }
        return 1;
    }
    for (size_t i = 0; i < t1->slot_types->length; i++) {
        if (type_structure_equality(get_ptr(access_list(t1->slot_types, i)), t2))
            return 1;
    }
    return 0;
}

int type_structure_equality(const Ang_Type *t1, const Ang_Type *t2)  {
    if (t1->id == t2->id) return 1;
    else if (t1->cat == PRIMITIVE) return 0;
    else if (t1->cat == SUM) return sum_type_structure_equality(t1, t2);
    else if (t1->cat == PRODUCT) {
        if (t2->cat != PRODUCT) return 0;
        for (size_t i = 0; i < t1->slot_types->length; i++) {
            const Ang_Type *slot_t1 = get_ptr(access_list(t1->slot_types, i));
            const Ang_Type *slot_t2 = get_ptr(access_list(t2->slot_types, i));
            if (!type_structure_equality(slot_t1, slot_t2)) return 0;
        }
        return 1;
    }
    return 0;

}

void add_slot(Ang_Type *type, const char *sym, const Ang_Type *slot_type) {
    int slot_num = type->slots->size;
    set_hashtable(type->slots, sym, from_double(slot_num));
    append_list(type->slot_types, from_ptr((void *) slot_type));
}

const Ang_Type *get_slot_type(const Ang_Type *type, int slot_num) {
    return get_ptr(access_list(type->slot_types, slot_num));
}
