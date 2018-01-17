#include "ang_primitives.h"

Ang_Type primitive_ang_type(int id) {
    switch (id) {
    case NUM_TYPE:
        return (Ang_Type) { id, "num", from_double(0) };
    case BOOL_TYPE:
        return (Ang_Type) { id, "bool", false_val };
    case STRING_TYPE:
        return (Ang_Type) { id, "string", from_ptr("") };
    }
    return (Ang_Type) { id, "undeclared", nil_val };
}
