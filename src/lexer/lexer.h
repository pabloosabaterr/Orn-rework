#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

enum token_type {
	TK_EOF,
	TK_ERROR,
	TK_ID,
	TK_NUMBER,
	TK_STRINGLIT,
	TK_CHARLIT,

	TK_INT,
	TK_UINT,
	TK_FLOAT,
	TK_DOUBLE,
	TK_BOOL,
	TK_VOID,
	TK_CHAR,
	TK_STRING,

	TK_IF,
	TK_ELIF,
	TK_ELSE,
	TK_LOOP,
	TK_RETURN,
	TK_BREAK,
	TK_CONTINUE,
	TK_STRUCT,
	TK_ENUM,
	TK_IMPORT,
	TK_TRUE,
	TK_FALSE,
	TK_LET,
	TK_CONST,
	TK_FN,
	TK_SIZE_OF,
	TK_SYSCALL,

	TK_LPAREN,
	TK_RPAREN,
	TK_LBRACE,
	TK_RBRACE,
	TK_LBRACKET,
	TK_RBRACKET,
	TK_SEMICOLON,
	TK_COMMA,
	TK_DOT,
	TK_EQUAL,
	TK_PLUS,
	TK_MINUS,
	TK_STAR,
	TK_SLASH,
	TK_LT,
	TK_GT,
	TK_LE,
	TK_GE,
	TK_EQEQ,
	TK_NEQ,
	TK_AMP,
	TK_PIPE,
	TK_CARET,
	TK_TILDE,
	TK_AND,
	TK_OR,
	TK_COLON,
	TK_NOT,
	TK_QUESTION,
	TK_PLUSPLUS,
	TK_MINUSMINUS,
	TK_LSHIFT,
	TK_RSHIFT,
	TK_LARROW,
	TK_RARROW,
	TK_MOD,
};

struct lexer_context {
	const char *src;
	const char *current;
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

void lexer_init(struct lexer_context *ctx, const char *src);
struct token token_next(struct lexer_context *ctx);
int dump_tokens(struct lexer_context *ctx);

#endif
