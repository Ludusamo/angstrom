#include "tokenizer.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include "error.h"

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
    char *lexeme = calloc(len + 1, sizeof(char));
    strncpy(lexeme, scanner->src + scanner->start, len);
    return lexeme;
}

void add_token(List *t_list, const Scanner *scanner,
               Token_Type t, const Value literal) {
    append_list(t_list,
        create_token_value(t, scanner, literal));
}

int match(Scanner *scanner, char c) {
    if (scanner->current > scanner->srclen) return 0;
    if (scanner->src[scanner->current] != c) return 0;
    scanner->current++;
    return 1;
}

char previous(const Scanner *scanner, int num_back) {
    assert(num_back > 0);
    if (scanner->current <= 0) return '\0';
    return scanner->src[scanner->current - num_back];
}

char peek(const Scanner *scanner) {
    if (scanner->current > scanner->srclen) return '\0';
    return scanner->src[scanner->current];
}

char peek_next(const Scanner *scanner) {
    if (scanner->current + 1 > scanner->srclen) return '\0';
    return scanner->src[scanner->current + 1];
}

Value string(Scanner *scanner) {
    while (peek(scanner) != '"' && scanner->current <= scanner->srclen) {
        if (peek(scanner) == '\n') scanner->line++;
        scanner->current++;
    }

    if (scanner->current > scanner->srclen) {
        error(scanner->line,
              UNTERMINATED_STRING,
              "String was not properly terminated");
        return nil_val;
    }

    scanner->current++;
    int len = scanner->current - scanner->start - 2;
    char *val = calloc(len + 1, sizeof(char));
    strncpy(val, scanner->src + scanner->start + 1, len);
    return create_token_value(STR, scanner, from_ptr(val));
}

Value number(Scanner *scanner) {
    int is_int = 0;
    if (previous(scanner, 2) == '.') is_int = 1;
    while (isdigit(peek(scanner))) scanner->current++;
    if (peek(scanner) == '.' && isdigit(peek_next(scanner)) && !is_int) {
        scanner->current++;
        while (isdigit(peek(scanner))) scanner->current++;
    }

    size_t len = scanner->current - scanner->start + 1;
    char val_str[len];
    strncpy(val_str, scanner->src + scanner->start, len);
    val_str[len - 1] = 0;
    double val = strtod(val_str, NULL);
    printf("%lf\n", val);
    return create_token_value(NUM, scanner, from_double(val));
}

void populate_keywords(Hashtable *keywords) {
    ctor_hashtable(keywords);
    set_hashtable(keywords, "var", from_double(VAR));
    set_hashtable(keywords, "func", from_double(FUNC));
    set_hashtable(keywords, "return", from_double(RETURN));
}

List *tokenize(const char *src) {
    Scanner scan = (Scanner) { src, strlen(src), 1, 0, 0 };

    Hashtable keywords;
    populate_keywords(&keywords);

    List *tokens = malloc(sizeof(List));
    ctor_list(tokens);
    while (scan.current < scan.srclen) {
        scan.start = scan.current;
        char c = src[scan.current++];
        switch (c) {
        case '(': add_token(tokens, &scan, LPAREN, nil_val); break;
        case ')': add_token(tokens, &scan, RPAREN, nil_val); break;
        case '{': add_token(tokens, &scan, LBRACE, nil_val); break;
        case '}': add_token(tokens, &scan, RBRACE, nil_val); break;
        case '[': add_token(tokens, &scan, LBRACK, nil_val); break;
        case ']': add_token(tokens, &scan, RBRACK, nil_val); break;
        case ',': add_token(tokens, &scan, COMMA, nil_val); break;
        case ':': add_token(tokens, &scan, COLON, nil_val); break;
        case '.': add_token(tokens, &scan, DOT, nil_val); break;
        case '-': add_token(tokens, &scan, MINUS, nil_val); break;
        case '+': add_token(tokens, &scan, PLUS, nil_val); break;
        case '/': add_token(tokens, &scan, SLASH, nil_val); break;
        case '*': add_token(tokens, &scan, STAR, nil_val); break;
        case '!':
            add_token(tokens, &scan, match(&scan, '=') ? NEQ : NOT, nil_val);
            break;
        case '=':
            add_token(tokens, &scan, match(&scan, '=') ? EQ_EQ : EQ, nil_val);
            break;
        case '<':
            add_token(tokens, &scan, match(&scan, '=') ? LTE : LT, nil_val);
            break;
        case '>':
            add_token(tokens, &scan, match(&scan, '=') ? GTE : GT, nil_val);
            break;
        case ' ':
        case '\r':
        case '\t':
            break;
        case '\n':
            scan.line++;
            break;
        case '"': {
            Value val = string(&scan);
            if (val.bits == nil_val.bits) {
                destroy_tokens(tokens);
                dtor_list(tokens);
                free(tokens);
                return 0;
            }
            append_list(tokens, val);
            break;
        }
        default:
            if (isdigit(c)) {
                Value val = number(&scan);
                append_list(tokens, val);
            } else if (isalpha(c)) {
                while (isalnum(peek(&scan))) scan.current++;
                const char *lexeme = copy_cur_lexeme(&scan);
                Value keyword = access_hashtable(&keywords, lexeme);
                Token_Type type = keyword.bits == nil_val.bits
                    ? IDENT : keyword.as_int32;
                free((void*) lexeme);
                add_token(tokens, &scan, type, nil_val);
            } else {
                error(scan.line, UNEXPECTED_CHARACTER, "Unexpected Character");
                destroy_tokens(tokens);
                dtor_list(tokens);
                free(tokens);
                return 0;
            }
        }
    }

    scan.start = scan.current++;
    dtor_hashtable(&keywords);
    add_token(tokens, &scan, TEOF, nil_val);
    return tokens;
}

void destroy_tokens(List *tokens) {
    for (size_t i = 0; i < tokens->length; i++) {
        Token *tok = (Token *) get_ptr(access_list(tokens, i));
        free((void *) tok->lexeme);
        if (is_ptr(tok->literal)) free(get_ptr(tok->literal));
        free(tok);
    }
    clear_list(tokens);
}
