#include "tokenizer.h"

#include <string.h>
#include <stdio.h>
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
    *t = (Token) {
        type,
        copy_cur_lexeme(scanner),
        literal,
        scanner->line,
        scanner->src_name
    };
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
    Value tok = create_token_value(t, scanner, literal);
    append_list(t_list, tok);
    #ifdef DEBUG
    print_token(get_ptr(tok));
    #endif
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
    set_hashtable(keywords, "var", from_double(VAR));
    set_hashtable(keywords, "func", from_double(FUNC));
    set_hashtable(keywords, "_", from_double(UNDERSCORE));
    set_hashtable(keywords, "type", from_double(TYPE_KEYWORD));
    set_hashtable(keywords, "return", from_double(RETURN));
}

const Token *last_scanned_token(const List *tokens) {
    return get_ptr(access_list(tokens, tokens->length - 1));
}

int tokenize(List *tokens, const char *src, const char *src_name) {
    size_t initial_length = tokens->length;
    Scanner scan = (Scanner) { src, strlen(src), 1, 0, 0, src_name };

    Hashtable keywords;
    populate_keywords(&keywords);

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
        case ':':
            add_token(tokens,
                &scan,
                match(&scan, ':') ? COLON_COLON : COLON,
                nil_val);
            break;
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
        case '|': add_token(tokens, &scan, PIPE, nil_val); break;
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
                destroy_tokens_from_n(tokens, initial_length);
                return 0;
            }
            add_token(tokens, &scan, STR, val);
            break;
        }
        default:
            if (isdigit(c)) {
                Value val = last_scanned_token(tokens)->type == DOT
                    ? integer(&scan)
                    : number(&scan);
                add_token(tokens, &scan, NUM, val);
            } else if (c == '_' || isalpha(c)) {
                while (peek(&scan) == '_' || isalnum(peek(&scan)))
                    scan.current++;
                const char *lexeme = copy_cur_lexeme(&scan);
                Value keyword = access_hashtable(&keywords, lexeme);
                Token_Type type = keyword.bits == nil_val.bits
                    ? IDENT : keyword.as_int32;
                free((void*) lexeme);
                add_token(tokens, &scan, type, nil_val);
            } else {
                error(scan.line, UNEXPECTED_CHARACTER, "Unexpected Character");
                destroy_tokens_from_n(tokens, initial_length);
                return 0;
            }
        }
    }

    scan.start = scan.current++;
    add_token(tokens, &scan, TEOF, nil_val);
    dtor_hashtable(&keywords);
    return 1;
}

void destroy_tokens_from_n(List *tokens, size_t n) {
    for (size_t i = n; i < tokens->length; i++) {
        Token *tok = (Token *) get_ptr(access_list(tokens, i));
        free((void *) tok->lexeme);
        if (is_ptr(tok->literal)) free(get_ptr(tok->literal));
        free(tok);
    }
    tokens->length = n;
}

void destroy_tokens(List *tokens) {
    destroy_tokens_from_n(tokens, 0);
}
