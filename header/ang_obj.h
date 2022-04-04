#ifndef ANG_OBJ_H
#define ANG_OBJ_H

#include "value.h"
#include "ang_type.h"

typedef struct Ang_Obj Ang_Obj;
struct Ang_Obj {
    Value v;
    const Ang_Type *type;
    int marked;
    Ang_Obj *next;
};

void mark_ang_obj(Ang_Obj *obj);
void print_ang_obj(const Value val);

#endif // ANG_OBJ_H
