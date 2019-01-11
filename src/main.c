#include <stdio.h>
#include "error.h"
#include "tokenizer.h"
#include "ast.h"
#include "ang_vm.h"
#include "ang_opcodes.h"
#include "ang_type.h"
#include "compiler.h"
#include "ang_primitives.h"

static char *get_line(void) {
    char * line = malloc(100), * linep = line;
    size_t lenmax = 100, len = lenmax;
    int c;

    if (line == NULL) return NULL;

    for (;;) {
        c = fgetc(stdin);
        if (c == EOF)
            break;

        if (--len == 0) {
            len = lenmax;
            char * linen = realloc(linep, lenmax *= 2);

            if (linen == NULL) {
                free(linep);
                return NULL;
            }
            line = linen + (line - linep);
            linep = linen;
        }

        if ((*line++ = c) == '\n') break;
    }
    *line = '\0';
    return linep;
}

static char *read_file(const char *file_path) {
    FILE *file = fopen(file_path, "r");

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    char *file_contents = malloc(file_size + 1);
    fread(file_contents, 1, file_size, file);
    file_contents[file_size] = 0;

    fclose(file);
    return file_contents;
}

void run_script(char *file_path) {
    Ang_VM vm;
    ctor_ang_vm(&vm, 100);
    #ifdef DEBUG
    vm.trace = 1;
    #endif
    Primitive_Types defaults;
    ctor_primitive_types(&defaults);
    set_hashtable(&vm.compiler.env.types, "Und", from_ptr(&defaults.und_default));
    set_hashtable(&vm.compiler.env.types, "Any", from_ptr(&defaults.und_default));
    set_hashtable(&vm.compiler.env.types, "Num", from_ptr(&defaults.num_default));
    set_hashtable(&vm.compiler.env.types, "Bool", from_ptr(&defaults.bool_default));
    set_hashtable(&vm.compiler.env.types, "String", from_ptr(&defaults.string_default));

    char *file_contents = read_file(file_path);
    run_code(&vm, file_contents, file_path);
    free(file_contents);

    dtor_ang_vm(&vm);
    dtor_primitive_types(&defaults);
    return;
}

void run_repl() {
    Ang_VM vm;
    ctor_ang_vm(&vm, 100);
    #ifdef DEBUG
    vm.trace = 1;
    #endif
    Primitive_Types defaults;
    ctor_primitive_types(&defaults);
    set_hashtable(&vm.compiler.env.types, "Und", from_ptr(&defaults.und_default));
    set_hashtable(&vm.compiler.env.types, "Any", from_ptr(&defaults.any_default));
    set_hashtable(&vm.compiler.env.types, "Num", from_ptr(&defaults.num_default));
    set_hashtable(&vm.compiler.env.types, "Bool", from_ptr(&defaults.bool_default));
    set_hashtable(&vm.compiler.env.types, "String", from_ptr(&defaults.string_default));
    char *expr;
    for (;;) {
        printf("> ");
        expr = get_line();
        if (strcmp(expr, "exit\n") == 0) {
            free(expr);
            break;
        }
        run_code(&vm, expr, "");
        if (!vm.enc_err) {
            Ang_Obj *result = pop_stack(&vm.mem);
            print_ang_obj(result);
            fprintf(stderr, "\n");
        } else {
            vm.enc_err = 0;
        }
        free(expr);
    }
    dtor_ang_vm(&vm);
    dtor_primitive_types(&defaults);
}

int main(int argc, char *argv[]) {
    if (argc > 2) {
        puts("Usage: angstrom [script]");
    } else if (argc == 2) {
        run_script(argv[1]);
    } else {
        run_repl();
    }

    return 0;
}

