#include <stdio.h>
#include "error.h"
#include "tokenizer.h"
#include "ast.h"
#include "ang_vm.h"

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
    Memory mem;
    ctor_memory(&mem, 100);

    for (int i = 0; i < 100; i++) {
        Ang_Obj *num = new_object(&mem, NUM);
        num->v = from_double(i);
        push_stack(&mem, num);
    }
    for (int i = 0; i < 50; i++) {
        pop_stack(&mem);
    }
    gc(&mem);
    printf("Num objects remaining: %zu\n", mem.num_objects);
    for (int i = 0; i < 50; i++) {
        pop_stack(&mem);
    }
    gc(&mem);

    dtor_memory(&mem);

    if (argc > 2) {
        puts("Usage: angstrom [script]");
    } else if (argc == 2) {
        run_script(argv[1]);
    } else {
        run_repl();
    }

    return 0;
}

