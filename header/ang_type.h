#ifndef ANG_TYPE_H
#define ANG_TYPE_H

#include "hashtable.h"

typedef enum {
    PRIMITIVE,
    SUM,
    PRODUCT
} Type_Category;

typedef struct {
    int id;
    const char *name;
    Type_Category cat;
    Value default_value;
    Hashtable *slots;
    List *slot_types;
    int user_defined;
} Ang_Type;

void ctor_ang_type(Ang_Type *type,
        int id,
        const char *name,
        Type_Category cat,
        Value default_val);
void dtor_ang_type(Ang_Type *type);

// Checks to see if t2 is the same type as t1
int type_equality(const Ang_Type *t1, const Ang_Type *t2);

void add_slot(Ang_Type *type, const char *sym, const Ang_Type *slot_type);

#endif // ANG_TYPE_H
