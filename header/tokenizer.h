#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <list.h>
#include <hashtable.h>
#include <value.h>

#define TOKEN_TYPE_LIST(code) \
    code(LPAREN) \
    code(RPAREN) \
    code(LBRACE) \
    code(RBRACE) \
    code(LBRACK) \
    code(RBRACK) \
    code(COMMA) \
    code(COLON) \
    code(COLON_COLON) \
    code(UNDERSCORE) \
    code(DOT) \
    code(MINUS) \
    code(PLUS) \
    code(SLASH) \
    code(STAR) \
    code(NOT) \
    code(NEQ) \
    code(EQ_EQ) \
    code(EQ) \
    code(GT) \
    code(GTE) \
    code(LT) \
    code(LTE) \
    code(PIPE) \
    code(THIN_ARROW) \
    code(ARROW) \
    code(IDENT) \
    code(STR) \
    code(NUM) \
    code(VAR) \
    code(FN) \
    code(MATCH) \
    code(TYPE_KEYWORD) \
    code(RETURN) \
    code(END)

#define DEFINE_ENUM_TYPE(type) TOKEN_##type,
typedef enum {
    TOKEN_TYPE_LIST(DEFINE_ENUM_TYPE)
} TokenType;
#undef DEFINE_ENUM_TYPE

typedef struct {
    TokenType type;
    char *lexeme;
    Value literal;
    int line;
    const char *src_name;
} Token;

typedef struct {
    const char *src;
    int srclen;
    int line;
    int start;
    int current;
    const char *src_name;
    Hashtable keywords;
} Scanner;

void ctor_scanner(Scanner *scanner, const char *src, const char *src_name);
void dtor_scanner(Scanner *scanner);

const char *token_type_to_str(TokenType t);
void print_token(const Token *t);
Token *create_token(const Scanner *scanner, TokenType type, const Value literal);
Value create_token_value(TokenType type,
                         const Scanner *scanner,
                         const Value literal);
char *copy_cur_lexeme(const Scanner *scanner);
void add_token(List *t_list, const Scanner *scanner,
               TokenType t, const Value literal);
int match(Scanner *scanner, char t);
char peek(const Scanner *scanner);
char peek_next(const Scanner *scanner);
Value string(Scanner *scanner);
Value number(Scanner *scanner);
void populate_keywords(Hashtable *keywords);
Token *scan_token(Scanner *scan);
Token *copy_token(const Token *token);

void destroy_tokens_from_n(List *tokens, size_t n);
void destroy_tokens(List *tokens);
#endif // TOKENIZER_H
