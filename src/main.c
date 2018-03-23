#include <stdio.h>
#include "error.h"
#include "tokenizer.h"
#include "ast.h"
#include "ang_vm.h"
#include "ang_opcodes.h"
#include "ang_type.h"
#include "compiler.h"
#include "ang_primitives.h"

char *get_line(void) {
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

        Primitive_Types defaults;
        ctor_primitive_types(&defaults);
        // Compile
        Compiler c;
        ctor_compiler(&c);
        set_hashtable(&c.env.types, "und", from_ptr(&defaults.und_default));
        set_hashtable(&c.env.types, "Num", from_ptr(&defaults.num_default));
        set_hashtable(&c.env.types, "Bool", from_ptr(&defaults.bool_default));
        set_hashtable(&c.env.types, "(", from_ptr(&defaults.tuple_default));
        compile(&c, ast);

        if (!c.enc_err) {
            printf("Beginning to execute\n");
            // Execute
            Ang_VM vm;
            ctor_ang_vm(&vm, 100);
            vm.trace = 1;
            run_compiled_instructions(&vm, &c);

            Ang_Obj *result = pop_stack(&vm.mem);
            print_ang_obj(result);

            dtor_ang_vm(&vm);
        }
        dtor_compiler(&c);
        dtor_primitive_types(&defaults);
    }

    // Cleanup
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
        expr = get_line();
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

