#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <list.h>
#include <value.h>

#define TOKEN_TYPE_LIST(code) \
    code(LPAREN) \
    code(RPAREN) \
    code(LBRACE) \
    code(RBRACE) \
    code(LBRACK) \
    code(RBRACK) \
    code(COMMA) \
    code(DOT) \
    code(MINUS) \
    code(PLUS) \
    code(SLASH) \
    code(STAR) \
    code(NOT) \
    code(NEQ) \
    code(EQ) \
    code(ASSIGN) \
    code(GT) \
    code(GTE) \
    code(LT) \
    code(LTE) \
    code(IDENT) \
    code(STR) \
    code(NUM) \
    code(FUNC) \
    code(RET) \
    code(TEOF)

#define DEFINE_ENUM_TYPE(type) type,
typedef enum {
    TOKEN_TYPE_LIST(DEFINE_ENUM_TYPE)
} Token_Type;
#undef DEFINE_ENUM_TYPE

typedef struct {
    Token_Type type;
    const char *lexeme;
    const Value literal;
    int line;
} Token;

const char *token_type_to_str(Token_Type t);
void print_token(const Token *t);
Value create_token_value(Token_Type type, const char *src, int start, int end,
                    const Value literal, int line);
const char *copy_lexeme(const char *src, int start, int end);
List *tokenize(const char *src);

#endif // TOKENIZER_H
