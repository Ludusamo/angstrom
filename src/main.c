#include <stdio.h>
#include "error.h"
#include "tokenizer.h"

void run(const char *exp) {
    List *tokens = tokenize(exp);
    for (size_t i = 0; i < tokens->length; i++) {
        Token *t = get_ptr(access_list(tokens, i));
        print_token(t);
    }
}

void run_script(char *file_path) {
    return;
}

void run_repl() {
    char exp[2048];
    for (;;) {
        printf("> ");
        scanf("%s", exp);
        run(exp);
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

