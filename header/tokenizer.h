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
    code(IDENT) \
    code(STR) \
    code(NUM) \
    code(VAR) \
    code(FUNC) \
    code(RETURN) \
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

typedef struct {
    const char *src;
    int srclen;
    int line;
    int start;
    int current;
} Scanner;

const char *token_type_to_str(Token_Type t);
void print_token(const Token *t);
Value create_token_value(Token_Type type,
                         const Scanner *scanner,
                         const Value literal);
const char *copy_cur_lexeme(const Scanner *scanner);
void add_token(List *t_list, const Scanner *scanner,
               Token_Type t, const Value literal);
int match(Scanner *scanner, char t);
char previous(const Scanner *scanner, int num_back);
char peek(const Scanner *scanner);
char peek_next(const Scanner *scanner);
Value string(Scanner *scanner);
Value number(Scanner *scanner);
void populate_keywords(Hashtable *keywords);
List *tokenize(const char *src);

void destroy_tokens(List *tokens);
#endif // TOKENIZER_H
