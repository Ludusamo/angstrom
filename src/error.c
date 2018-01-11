#include "error.h"

#include <stdio.h>

#define DEFINE_CODE_STRING(type) case type: return #type;
const char *error_code_to_str(Error_Code c) {
    switch (c) {
        ERROR_LIST(DEFINE_CODE_STRING)
    }
}
#undef DEFINE_CODE_STRING

void error(int line, Error_Code c, const char *message) {
    fprintf(stderr, "[line %d] Error %s: %s",
        line, error_code_to_str(c), message);
}

void runtime_error(Error_Code c, const char *message) {
    fprintf(stderr, "Runtime Error %s: %s", error_code_to_str(c), message);
}
