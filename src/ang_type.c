#include "ang_type.h"

#include "ang_primitives.h"

void ctor_ang_type(Ang_Type *type, int id, const char *name, Value default_val) {
    *type = (Ang_Type) { id, name, default_val };
    ctor_hashtable(&type->slots);
}

void dtor_ang_type(Ang_Type *type) {
    if (type->id != TUPLE_TYPE) {
        // Clear out slots
        Iter slots_iter;
        iter_hashtable(&slots_iter, &type->slots);
        foreach(slots_iter) {
            Keyval *kv = get_ptr(val_iter_hashtable(&slots_iter));
            free((void *) (kv->key));
            free(get_ptr(kv->val));
        }
        destroy_iter_hashtable(&slots_iter);
    }

    dtor_hashtable(&type->slots);
}
