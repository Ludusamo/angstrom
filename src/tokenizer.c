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

Value create_token_value(Token_Type type, const char *src, int start, int end,
                    const Value literal, int line) {
    Token *t = malloc(sizeof(Token));
    *t = (Token) { type, copy_lexeme(src, start, end), literal, line };
    return from_ptr((void *) t);
}

const char *copy_lexeme(const char *src, int start, int end) {
    size_t len = end - start;
    char *lexeme = calloc(len, sizeof(char));
    strncpy(lexeme, src + start, len);
    return lexeme;
}

List *tokenize(const char *src) {
    size_t srclen = strlen(src);
    int start = 0;
    int current = 0;
    int line = 1;

    List *tokens = malloc(sizeof(List));
    ctor_list(tokens);
    while (current <= srclen) {
        start = current;
        char c = src[current++];
        switch (c) {
        case '(':
            append_list(tokens,
                create_token_value(LPAREN, src, start, current, nil_val, line));
            break;
        case ')':
            append_list(tokens,
                create_token_value(RPAREN, src, start, current, nil_val, line));
            break;
        case '{':
            append_list(tokens,
                create_token_value(LBRACE, src, start, current, nil_val, line));
            break;
        case '}':
            append_list(tokens,
                create_token_value(RBRACE, src, start, current, nil_val, line));
            break;
        case '[':
            append_list(tokens,
                create_token_value(LBRACK, src, start, current, nil_val, line));
            break;
        case ']':
            append_list(tokens,
                create_token_value(RBRACK, src, start, current, nil_val, line));
            break;
        case ',':
            append_list(tokens,
                create_token_value(COMMA, src, start, current, nil_val, line));
            break;
        case '.':
            append_list(tokens,
                create_token_value(DOT, src, start, current, nil_val, line));
            break;
        case '-':
            append_list(tokens,
                create_token_value(MINUS, src, start, current, nil_val, line));
            break;
        case '+':
            append_list(tokens,
                create_token_value(PLUS, src, start, current, nil_val, line));
            break;
        case '*':
            append_list(tokens,
                create_token_value(STAR, src, start, current, nil_val, line));
            break;
        }
    }
    return tokens;
}
