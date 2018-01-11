#include <stdio.h>
#include "error.h"
#include "tokenizer.h"
#include "ast.h"
#include "ang_vm.h"
#include "ang_opcodes.h"

char *getline(void) {
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

void run(const char *exp) {
    List *tokens = tokenize(exp);
    if (tokens == 0) {
        printf("\n");
        return;
    }
    for (size_t i = 0; i < tokens->length; i++) {
        Token *t = get_ptr(access_list(tokens, i));
        print_token(t);
    }
    Ast *ast = parse(tokens);
    if (ast) {
        print_ast(ast, 0);
        destroy_ast(ast);
        free(ast);
    }
    destroy_tokens(tokens);
    dtor_list(tokens);
    free(tokens);
}

void run_script(char *file_path) {
    return;
}

void run_repl() {
    char *expr;
    for (;;) {
        printf("> ");
        expr = getline();
        run(expr);
        free(expr);
    }
}

int main(int argc, char *argv[]) {
    Ang_VM vm;
    ctor_ang_vm(&vm, 100);
    vm.trace = 1;
    vm.running = 1;
    int instr[] = {
        PUSH, 10,
        PUSH, 3,
        ADD,
        PUSH, 4,
        DIV,
        HALT
    };
    for (int i = 0; i < (sizeof(instr) / sizeof(int)); i++) {
        emit_op(&vm, instr[i]);
        printf("%d\n", access_list(&vm.prog, i).as_int32);
    }
    printf("Evaluating test prog\n");
    printf("%d\n", vm.mem.registers[IP]);
    while (vm.running) eval(&vm);
    dtor_ang_vm(&vm);

    if (argc > 2) {
        puts("Usage: angstrom [script]");
    } else if (argc == 2) {
        run_script(argv[1]);
    } else {
        run_repl();
    }

    return 0;
}

