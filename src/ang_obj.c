#include "ang_obj.h"

#include "ang_primitives.h"
#include "stdio.h"

void mark_ang_obj(Ang_Obj *obj) {
    obj->marked = 1;
}

void print_ang_obj(const Ang_Obj *obj) {
    if (obj->type == NUM_TYPE) {
        if (is_double(obj->v)) printf("%lf\n", obj->v.as_double);
        else printf("%d\n", obj->v.as_int32);
    } else if (obj->type == STRING_TYPE) {
        // TODO: Print String
    }
}
