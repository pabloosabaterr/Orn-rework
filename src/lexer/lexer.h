#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

struct compiler_context;
struct diag_context;

enum token_type {
    TK_EOF,
    TK_ERROR,
    TK_ID,

    TK_NUMBER,
    TK_FLOATING, /* A floating point literal unresolved size */
    TK_HEX,
    TK_OCTAL,
    TK_BINARY,
    TK_STRINGLIT,
    TK_CHARLIT,

    TK_IN,
    TK_UN,
    TK_FL,
    TK_DB,
    TK_BL,
    TK_CH,
    TK_ST,
    TK_VD,
    TK_TP,

    TK_OBJ,
    TK_ENUM,
    TK_IF,
    TK_ELSE,
    TK_LOOP,
    TK_RETURN,
    TK_BREAK,
    TK_CONTINUE,
    TK_GOTO,
    TK_IMPORT,
    TK_TRUE,
    TK_FALSE,
    TK_NULL,

    TK_LPAREN,
    TK_RPAREN,
    TK_LBRACE,
    TK_RBRACE,
    TK_LBRACKET,
    TK_RBRACKET,
    TK_SEMICOLON,
    TK_COMMA,
    TK_DOT,
    TK_COLON,
    TK_UNDERSCORE,
    TK_HASH,

    TK_DECL,
    TK_WALRUS,
    TK_EQUAL,
    TK_PLUSEQ,
    TK_MINUSEQ,
    TK_STAREQ,
    TK_SLASHEQ,
    TK_MODEQ,

    TK_PLUS,
    TK_MINUS,
    TK_STAR,
    TK_SLASH,
    TK_MOD,

    TK_LT,
    TK_GT,
    TK_LE,
    TK_GE,
    TK_CMP,
    TK_NEQ,
    TK_AND,
    TK_OR,
    TK_NOT,

    TK_AMP,
    TK_PIPE,
    TK_CARET,
    TK_TILDE,
    TK_LSHIFT,
    TK_RSHIFT,

    TK_RANGE,
    TK_SPREAD,

    TK_COUNT,
};

struct lexer_context {
    const char *src;
    const char *current;
    const char *file;
    struct diag_context *diag;
    int line;
    int col;
};

struct token {
    enum token_type type;
    const char *lex;
    size_t len;
    int line;
    int col;
};

void lexer_init(struct lexer_context *ctx, struct compiler_context *cc);
struct token token_next(struct lexer_context *ctx);
int dump_tokens(struct lexer_context *ctx);
const char *token_type_pretty(enum token_type type);

#endif
