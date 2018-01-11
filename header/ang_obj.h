#ifndef ANG_OBJ_H
#define ANG_OBJ_H

#include "value.h"

typedef struct Ang_Obj Ang_Obj;
struct Ang_Obj {
    Value v;
    int type;
    int marked;
    Ang_Obj *next;
};

void mark_ang_obj(Ang_Obj *obj);

#endif // ANG_OBJ_H
