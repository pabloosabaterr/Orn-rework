#include "lexer/lexer.h"
#include "memory/wrapper.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

struct keyword {
	enum token_type type;
	const char *str;
};

static const struct keyword keyword_table[] = {
	{ TK_INT, "int" },
	{ TK_UINT, "unsigned" },
	{ TK_FLOAT, "float" },
	{ TK_DOUBLE, "double" },
	{ TK_BOOL, "bool" },
	{ TK_VOID, "void" },
	{ TK_CHAR, "char" },
	{ TK_STRING, "string" },
	{ TK_IF, "if" },
	{ TK_ELIF, "elif" },
	{ TK_ELSE, "else" },
	{ TK_LOOP, "while" },
	{ TK_RETURN, "ret" },
	{ TK_BREAK, "break" },
	{ TK_CONTINUE, "continue" },
	{ TK_STRUCT, "struct" },
	{ TK_ENUM, "enum" },
	{ TK_IMPORT, "import" },
	{ TK_TRUE, "true" },
	{ TK_FALSE, "false" },
	{ TK_LET, "let" },
	{ TK_CONST, "const" },
	{ TK_FN, "fn" },
	{ TK_SIZE_OF, "sizeof" },
	{ TK_SYSCALL, "syscall" },
	{ TK_FOR, "for" },
	{ TK_IN, "in" },
	{ TK_MATCH, "match" },
	{ TK_DEFER, "defer" },
	{ TK_IMPL, "impl" },
	{ TK_TYPE, "type" },
	{ TK_AS, "as" },
	{ TK_NULL, "null" },
};

static inline int is_next(struct lexer_context *lexer, char c)
{
	return *lexer->current == c;
}
#define is_end(lexer) (is_next(lexer, '\0'))
#define create_token(type, start, len) \
	((struct token){ type, start, len, lexer->line, lexer->col - (int)(len) })

static char advance(struct lexer_context *lexer)
{
	char c = *lexer->current++;
	if (c == '\n') {
		lexer->line++;
		lexer->col = 0;
	} else {
		lexer->col++;
	}
	return c;
}

static inline int follow_next(struct lexer_context *lexer, char c)
{
	if (is_next(lexer, c)) {
		advance(lexer);
		return 1;
	}
	return 0;
}

static int try_ignore_comments(struct lexer_context *lexer)
{
	if (is_next(lexer, '/')) {
		advance(lexer);
		while (!is_end(lexer) && !is_next(lexer, '\n'))
			advance(lexer);
		return 1;
	} else if (is_next(lexer, '*')) {
		advance(lexer);
		while (!is_end(lexer)) {
			if (follow_next(lexer, '*') && follow_next(lexer, '/'))
				return 1;
			advance(lexer);
		}
		die("unterminated block comment");
	}
	return 0;
}

static void skip_noise(struct lexer_context *lexer)
{
	while (!is_end(lexer)) {
		switch (*lexer->current) {
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			advance(lexer);
			break;
		case '/':
			lexer->current++;
			if (!try_ignore_comments(lexer)) {
				lexer->current--;
				return;
			};
			break;
		default:
			return;
		}
	}
}

static struct token token_id(struct lexer_context *lexer, const char *start)
{
	size_t i, len;

	while (!is_end(lexer) && (isalnum(*lexer->current) || is_next(lexer, '_')))
		advance(lexer);

	len = lexer->current - start;

	for (i = 0; i < ARRAY_SIZE(keyword_table); i++)
		if (strlen(keyword_table[i].str) == len &&
		    !memcmp(start, keyword_table[i].str, len))
			return create_token(keyword_table[i].type, start, len);

	return create_token(TK_ID, start, len);
}

static struct token token_number(struct lexer_context *lexer, const char *start)
{
	while (!is_end(lexer) && isdigit(*lexer->current))
		advance(lexer);

	return create_token(TK_NUMBER, start, (size_t)(lexer->current - start));
}

static struct token error_token(struct lexer_context *lexer, const char *start, int len, const char *msg)
{
	fprintf(stderr, "lexer error [%d:%d]: %s\n", lexer->line, lexer->col, msg);
	return (struct token){ TK_ERROR, start, len, lexer->line, lexer->col - (int)(len) };
}

static struct token token_stringlit(struct lexer_context *lexer, const char *start)
{
	while (!is_end(lexer) && !is_next(lexer, '"')) {
		if (is_next(lexer, '\\'))
			advance(lexer);
		advance(lexer);
	}

	if (is_end(lexer))
		return error_token(lexer, start, lexer->current - start, "unterminated string");

	advance(lexer);

	return create_token(TK_STRINGLIT, start, (size_t)(lexer->current - start));
}

static struct token token_charlit(struct lexer_context *lexer, const char *start)
{
	if (is_next(lexer, '\\'))
		advance(lexer);

	advance(lexer);

	if (!is_next(lexer, '\''))
		return error_token(lexer, start, lexer->current - start, "unterminated char literal");

	advance(lexer);

	return create_token(TK_CHARLIT, start, (size_t)(lexer->current - start));
}

void lexer_init(struct lexer_context *lexer, const char *src)
{
	lexer->src = src;
	lexer->current = src;
	lexer->line = 1;
	lexer->col = 0;
}

struct token token_next(struct lexer_context *lexer)
{
	const char *start;
	char c;

	skip_noise(lexer);

	if (is_end(lexer))
		return create_token(TK_EOF, lexer->current, 0);

	start = lexer->current;
	c = advance(lexer);
	switch (c) {
	case '(':
		return create_token(TK_LPAREN, start, 1);
	case ')':
		return create_token(TK_RPAREN, start, 1);
	case '{':
		return create_token(TK_LBRACE, start, 1);
	case '}':
		return create_token(TK_RBRACE, start, 1);
	case '[':
		return create_token(TK_LBRACKET, start, 1);
	case ']':
		return create_token(TK_RBRACKET, start, 1);
	case ';':
		return create_token(TK_SEMICOLON, start, 1);
	case ',':
		return create_token(TK_COMMA, start, 1);
	case '_':
		return create_token(TK_UNDERSCORE, start, 1);
	case '.':
		if (follow_next(lexer, '.')) {
			if (follow_next(lexer, '.'))
				return create_token(TK_SPREAD, start, 3);
			return create_token(TK_RANGE, start, 2);
		}
		return create_token(TK_DOT, start, 1);
	case '~':
		return create_token(TK_TILDE, start, 1);
	case '?':
		return create_token(TK_QUESTION, start, 1);
	case ':':
		if (follow_next(lexer, ':'))
			return create_token(TK_NAMESPACE, start, 2);
		return create_token(TK_COLON, start, 1);
	case '%':
		if (follow_next(lexer, '='))
			return create_token(TK_MODEQ, start, 2);
		return create_token(TK_MOD, start, 1);
	case '*':
		if (follow_next(lexer, '='))
			return create_token(TK_STAREQ, start, 2);
		return create_token(TK_STAR, start, 1);
	case '^':
		return create_token(TK_CARET, start, 1);
	case '/':
		if (follow_next(lexer, '='))
			return create_token(TK_SLASHEQ, start, 2);
		return create_token(TK_SLASH, start, 1);

	case '+':
		if (follow_next(lexer, '+'))
			return create_token(TK_INCREMENT, start, 2);
		if (follow_next(lexer, '='))
			return create_token(TK_PLUSEQ, start, 2);
		return create_token(TK_PLUS, start, 1);
	case '-':
		if (follow_next(lexer, '-'))
			return create_token(TK_DECREMENT, start, 2);
		if (follow_next(lexer, '>'))
			return create_token(TK_RARROW, start, 2);
		if (follow_next(lexer, '='))
			return create_token(TK_MINUSEQ, start, 2);
		return create_token(TK_MINUS, start, 1);
	case '=':
		if (follow_next(lexer, '='))
			return create_token(TK_CMP, start, 2);
		if (follow_next(lexer, '>'))
			return create_token(TK_FATARROW, start, 2);
		return create_token(TK_EQUAL, start, 1);
	case '!':
		if (follow_next(lexer, '='))
			return create_token(TK_NEQ, start, 2);
		return create_token(TK_NOT, start, 1);
	case '&':
		if (follow_next(lexer, '&'))
			return create_token(TK_AND, start, 2);
		return create_token(TK_AMP, start, 1);
	case '|':
		if (follow_next(lexer, '|'))
			return create_token(TK_OR, start, 2);
		return create_token(TK_PIPE, start, 1);
	case '<':
		if (follow_next(lexer, '<'))
			return create_token(TK_LSHIFT, start, 2);
		else if (follow_next(lexer, '='))
			return create_token(TK_LE, start, 2);
		else if (follow_next(lexer, '-'))
			return create_token(TK_LARROW, start, 2);
		return create_token(TK_LT, start, 1);
	case '>':
		if (follow_next(lexer, '>'))
			return create_token(TK_RSHIFT, start, 2);
		else if (follow_next(lexer, '='))
			return create_token(TK_GE, start, 2);
		return create_token(TK_GT, start, 1);

	case '"':
		return token_stringlit(lexer, start);
	case '\'':
		return token_charlit(lexer, start);

	default:
		if (isalpha(c) || c == '_')
			return token_id(lexer, start);
		if (isdigit(c))
			return token_number(lexer, start);
		return error_token(lexer, start, 1, "unexpected character");
	}
}

int dump_tokens(struct lexer_context *lexer)
{
	struct token token;
	int errors = 0;

	while ((token = token_next(lexer)).type != TK_EOF) {
		if (token.type == TK_ERROR) {
			errors++;
			continue;
		}
		printf("%.*s - %d:%d\n", (int)token.len, token.lex,
		       token.line, token.col);
	}

	return errors;
}
