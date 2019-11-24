#include "ang_obj.h"

#include "ang_primitives.h"
#include "lambda.h"
#include "stdio.h"

void mark_ang_obj(Ang_Obj *obj) {
    if (!obj || obj->marked == 1) return;
    obj->marked = 1;
    if (obj->type->cat == PRODUCT || obj->type->cat == ARRAY) {
        List *slots = get_ptr(obj->v);
        for (size_t i = 0; i < slots->length; i++) {
            mark_ang_obj(get_ptr(access_list(slots, i)));
        }
    } else if (obj->type->cat == LAMBDA) {
        Lambda *l = get_ptr(obj->v);
        for (int i = 0; i < l->nenv; i++) {
            mark_ang_obj(l->env[i]);
        }
    }
}

void print_ang_obj(const Ang_Obj *obj) {
    Value v = obj->v;
    const Ang_Type *t = obj->type;
    if (t->id == BOOL_TYPE) {
        fprintf(stderr, "%s ", v.bits == true_val.bits
            ? "true"
            : "false");
    } else if (t->cat == ARRAY) {
        fprintf(stderr, "[ ");
        List *slots = get_ptr(v);
        for (size_t i = 0; i < slots->length; i++) {
            print_ang_obj(get_ptr(access_list(slots, i)));
            if (i + 1 != slots->length) fprintf(stderr, ", ");
        }
        fprintf(stderr, " ]");
    } else if (is_double(v)) {
        fprintf(stderr, "%.2lf", v.as_double);
    } else if (is_int32(v))
        fprintf(stderr, "%d", v.as_int32);
    else if (v.bits == nil_val.bits)
        fprintf(stderr, "NULL");
    else if (is_ptr(v)) {
        fprintf(stderr, "<%s>", t->name);
    }
}
