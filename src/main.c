#include <stdio.h>
#include "error.h"
#include "tokenizer.h"
#include "ast.h"
#include "ang_vm.h"
#include "ang_opcodes.h"
#include "compiler.h"

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
    // Tokenize
    List *tokens = tokenize(exp);
    if (tokens == 0) {
        printf("\n");
        return;
    }
    for (size_t i = 0; i < tokens->length; i++) {
        Token *t = get_ptr(access_list(tokens, i));
        print_token(t);
    }

    // Parse
    Parser parser = (Parser) { 0, tokens, 0 };
    Ast *ast = parse_expression(&parser);
    if (ast) {
        print_ast(ast, 0);
    }

    // Compile
    Compiler c;
    ctor_compiler(&c);
    compile(&c, ast);
    for (size_t i = 0; i < c.instr.length; i++) {
        printf("%d\n", access_list(&c.instr, i).as_int32);
    }

    // Execute
    Ang_VM vm;
    ctor_ang_vm(&vm, 100);
    vm.trace = 1;
    for (size_t i = 0; i < c.instr.length; i++) {
        emit_op(&vm, access_list(&c.instr, i).as_int32);
    }
    printf("%d\n", vm.mem.registers[IP]);
    while (vm.mem.registers[IP] < vm.prog.length) eval(&vm);

    // Cleanup
    dtor_ang_vm(&vm);
    dtor_compiler(&c);
    destroy_ast(ast);
    free(ast);
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
    if (argc > 2) {
        puts("Usage: angstrom [script]");
    } else if (argc == 2) {
        run_script(argv[1]);
    } else {
        run_repl();
    }

    return 0;
}

