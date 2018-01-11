#include "ang_opcodes.h"

#define DEFINE_CODE_STRING(type, _) case type: return #type;
const char *opcode_to_str(Opcode op) {
    switch (op) {
        OPCODES(DEFINE_CODE_STRING)
    }
}
#undef DEFINE_CODE_STRING

#define DEFINE_CODE_NUM_OPS(type, num_ops) case type: return num_ops;
int num_ops(Opcode op) {
    switch (op) {
        OPCODES(DEFINE_CODE_NUM_OPS)
    }
}
#undef DEFINE_CODE_NUM_OPS
