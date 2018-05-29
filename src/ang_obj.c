#include "ang_obj.h"

#include "ang_primitives.h"
#include "stdio.h"

void mark_ang_obj(Ang_Obj *obj) {
    obj->marked = 1;
    if (obj->type->cat == PRODUCT) {
        List *slots = get_ptr(obj->v);
        for (size_t i = 0; i < slots->length; i++) {
            mark_ang_obj(get_ptr(access_list(slots, i)));
        }
    }
}

void print_ang_obj(const Ang_Obj *obj) {
    if (obj->type->id == NUM_TYPE) {
        if (is_double(obj->v)) printf("%lf\n", obj->v.as_double);
        else printf("%d\n", obj->v.as_int32);
    } else if (obj->type->id == STRING_TYPE) {
        // TODO: Print String
    } else {
        printf("<%s>\n", obj->type->name);
    }
}
