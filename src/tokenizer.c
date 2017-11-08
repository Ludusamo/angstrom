#include "tokenizer.h"

#include <string.h>
#include <stdio.h>

#define DEFINE_CODE_STRING(type) case type: return #type;
const char *token_type_to_str(Token_Type t) {
    switch (t) {
        TOKEN_TYPE_LIST(DEFINE_CODE_STRING)
    }
}
#undef DEFINE_CODE_STRING

void print_token(const Token *t) {
    printf("{ %s, %s, %d }\n", token_type_to_str(t->type), t->lexeme, t->line);
}

Value create_token_value(Token_Type type,
                         const Scanner *scanner,
                         const Value literal) {
    Token *t = malloc(sizeof(Token));
    *t = (Token) { type, copy_cur_lexeme(scanner), literal, scanner->line };
    return from_ptr((void *) t);
}

const char *copy_cur_lexeme(const Scanner *scanner) {
    size_t len = scanner->current - scanner->start;
    char *lexeme = calloc(len, sizeof(char));
    strncpy(lexeme, scanner->src + scanner->start, len);
    return lexeme;
}

void add_token(List *t_list, const Scanner *scanner,
               Token_Type t, const Value literal) {
    append_list(t_list,
        create_token_value(t, scanner, literal));
}

List *tokenize(const char *src) {
    Scanner scanner = (Scanner) { src, strlen(src), 1, 0, 0 };

    List *tokens = malloc(sizeof(List));
    ctor_list(tokens);
    while (scanner.current <= scanner.srclen) {
        scanner.start = scanner.current;
        char c = src[scanner.current++];
        switch (c) {
        case '(': add_token(tokens, &scanner, LPAREN, nil_val); break;
        case ')': add_token(tokens, &scanner, RPAREN, nil_val); break;
        case '{': add_token(tokens, &scanner, LBRACE, nil_val); break;
        case '}': add_token(tokens, &scanner, RBRACE, nil_val); break;
        case '[': add_token(tokens, &scanner, LBRACK, nil_val); break;
        case ']': add_token(tokens, &scanner, RBRACK, nil_val); break;
        case ',': add_token(tokens, &scanner, COMMA, nil_val); break;
        case '.': add_token(tokens, &scanner, DOT, nil_val); break;
        case '-': add_token(tokens, &scanner, MINUS, nil_val); break;
        case '+': add_token(tokens, &scanner, PLUS, nil_val); break;
        case '*': add_token(tokens, &scanner, STAR, nil_val); break;
        }
    }
    return tokens;
}
