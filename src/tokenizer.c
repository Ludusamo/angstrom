#include "tokenizer.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "error.h"

void ctor_scanner(Scanner *scanner, const char *src, const char *src_name) {
    scanner->src = src;
    scanner->srclen = strlen(src);
    scanner->line = 1;
    scanner->start = 0;
    scanner->current = 0;
    scanner->src_name = src_name;
    ctor_hashtable(&scanner->keywords);
    populate_keywords(&scanner->keywords);
}

void dtor_scanner(Scanner *scanner) {
    dtor_hashtable(&scanner->keywords);
}

#define DEFINE_CODE_STRING(type) case TOKEN_##type: return #type;
const char *token_type_to_str(TokenType t) {
    switch (t) {
        TOKEN_TYPE_LIST(DEFINE_CODE_STRING)
    }
}
#undef DEFINE_CODE_STRING

void print_token(const Token *t) {
    printf("{ %s, %s, %d }\n", token_type_to_str(t->type), t->lexeme, t->line);
}

Token *create_token(const Scanner *scanner,
                   TokenType type,
                   const Value literal) {
    Token *tok = malloc(sizeof(Token));
    *tok = (Token) {
        type,
        copy_cur_lexeme(scanner),
        literal,
        scanner->line,
        scanner->src_name
    };
    return tok;
}

char *copy_cur_lexeme(const Scanner *scanner) {
    size_t len = scanner->current - scanner->start;
    char *lexeme = calloc(len + 1, sizeof(char));
    strncpy(lexeme, scanner->src + scanner->start, len);
    return lexeme;
}

int match(Scanner *scanner, char c) {
    if (scanner->current > scanner->srclen) return 0;
    if (scanner->src[scanner->current] != c) return 0;
    scanner->current++;
    return 1;
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
    return from_ptr(val);
}

Value integer(Scanner *scanner) {
    while (isdigit(peek(scanner))) scanner->current++;
    double val = strtod(scanner->src + scanner->start, NULL);
    return from_double(val);
}

Value number(Scanner *scanner) {
    while (isdigit(peek(scanner))) scanner->current++;

    // If this number is in reference to an accessor, don't scan for decimals
    if (peek(scanner) == '.' && isdigit(peek_next(scanner))) {
        scanner->current++;
        while (isdigit(peek(scanner))) scanner->current++;
    }
    double val = strtod(scanner->src + scanner->start, NULL);
    return from_double(val);
}

void populate_keywords(Hashtable *keywords) {
    ctor_hashtable(keywords);
    set_hashtable(keywords, "var", from_double(TOKEN_VAR));
    set_hashtable(keywords, "val", from_double(TOKEN_VAL));
    set_hashtable(keywords, "fn", from_double(TOKEN_FN));
    set_hashtable(keywords, "_", from_double(TOKEN_UNDERSCORE));
    set_hashtable(keywords, "type", from_double(TOKEN_TYPE_KEYWORD));
    set_hashtable(keywords, "return", from_double(TOKEN_RETURN));
    set_hashtable(keywords, "match", from_double(TOKEN_MATCH));
    set_hashtable(keywords, "true", from_double(TOKEN_TRUE));
    set_hashtable(keywords, "false", from_double(TOKEN_FALSE));
}

Token *scan_token(Scanner *scan) {
    while (scan->current < scan->srclen) {
        scan->start = scan->current;
        char c = scan->src[scan->current++];
        switch (c) {
        case '(': return create_token(scan, TOKEN_LPAREN, nil_val);
        case ')': return create_token(scan, TOKEN_RPAREN, nil_val);
        case '{': return create_token(scan, TOKEN_LBRACE, nil_val);
        case '}': return create_token(scan, TOKEN_RBRACE, nil_val);
        case '[': return create_token(scan, TOKEN_LBRACK, nil_val);
        case ']': return create_token(scan, TOKEN_RBRACK, nil_val);
        case ',': return create_token(scan, TOKEN_COMMA, nil_val);
        case ':':
            return create_token(scan,
                match(scan, ':') ? TOKEN_COLON_COLON : TOKEN_COLON,
                nil_val);
        case '.': return create_token(scan, TOKEN_DOT, nil_val);
        case '-':
            if (match(scan, '>')) {
                return create_token(scan, TOKEN_THIN_ARROW, nil_val);
            } else {
                return create_token(scan, TOKEN_MINUS, nil_val);
            }
        case '+': return create_token(scan, TOKEN_PLUS, nil_val);
        case '/': return create_token(scan, TOKEN_SLASH, nil_val);
        case '*': return create_token(scan, TOKEN_STAR, nil_val);
        case '&':
            if (match(scan, '&'))
                return create_token(scan, TOKEN_AND, nil_val);
        case '!':
            return create_token(scan,
                    match(scan, '=') ? TOKEN_NEQ : TOKEN_NOT, nil_val);
        case '=':
            if (match(scan, '=')) {
                return create_token(scan, TOKEN_EQ_EQ, nil_val);
            } else if (match(scan, '>')) {
                return create_token(scan, TOKEN_ARROW, nil_val);
            } else {
                return create_token(scan, TOKEN_EQ, nil_val);
            }
        case '<':
            return create_token(scan,
                match(scan, '=') ? TOKEN_LTE : TOKEN_LT, nil_val);
        case '>':
            return create_token(scan,
                match(scan, '=') ? TOKEN_GTE : TOKEN_GT, nil_val);
        case '|':
            if (match(scan, '|')) return create_token(scan, TOKEN_OR, nil_val);
            else return create_token(scan, TOKEN_PIPE, nil_val);
        case ' ':
        case '\r':
        case '\t':
            break;
        case '\n':
            scan->line++;
            break;
        case '"': {
            Value val = string(scan);
            //if (val.bits == nil_val.bits) {
            //    destroy_tokens_from_n(tokens, initial_length);
            //    return 0;
            //}
            return create_token(scan, TOKEN_STR, val);
        }
        default:
            if (isdigit(c)) {
                Value val = number(scan);
                return create_token(scan, TOKEN_NUM, val);
            } else if (c == '_' || isalpha(c)) {
                while (peek(scan) == '_' || isalnum(peek(scan)))
                    scan->current++;
                const char *lexeme = copy_cur_lexeme(scan);
                Value keyword = access_hashtable(&scan->keywords, lexeme);
                TokenType type = keyword.bits == nil_val.bits
                    ? TOKEN_IDENT : keyword.as_int32;
                free((void*) lexeme);
                return create_token(scan, type, nil_val);
            }
        }
    }

    scan->current++;
    return create_token(scan, TOKEN_END, nil_val);
}

Token *copy_token(const Token *token) {
    Token *cpy = malloc(sizeof(Token));
    *cpy = *token;
    cpy->lexeme = calloc(strlen(token->lexeme) + 1, sizeof(char));
    strcpy(cpy->lexeme, token->lexeme);
    return cpy;
}
