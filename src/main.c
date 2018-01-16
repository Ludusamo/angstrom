#include <stdio.h>
#include "error.h"
#include "tokenizer.h"
#include "ast.h"
#include "ang_vm.h"
#include "ang_opcodes.h"
#include "ang_type.h"
#include "compiler.h"
#include "ang_primitives.h"

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
    if (!parser.enc_err) {

        // Compile
        Compiler c;
        ctor_compiler(&c);
        Ang_Type und = primitive_ang_type(UNDECLARED);
        set_hashtable(&c.env.types, "undeclared", from_ptr(&und));
        Ang_Type num = primitive_ang_type(NUM_TYPE);
        set_hashtable(&c.env.types, "num", from_ptr(&num));
        compile(&c, ast);

        // Execute
        Ang_VM vm;
        ctor_ang_vm(&vm, 100);
        vm.trace = 1;
        run_compiled_instructions(&vm, &c);

        Ang_Obj *result = pop_stack(&vm.mem);
        print_ang_obj(result);

        dtor_ang_vm(&vm);
        dtor_compiler(&c);
        destroy_ast(ast);
        free(ast);
    }

    // Cleanup
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

