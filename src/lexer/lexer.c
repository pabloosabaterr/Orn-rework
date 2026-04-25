#include "lexer/lexer.h"
#include "diagnostic/diagnostic.h"
#include "memory/wrapper.h"
#include "compiler.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const char *token_type_names[] = {
	[TK_EOF] = "EOF",
	[TK_ERROR] = "ERROR",
	[TK_ID] = "ID",
	[TK_NUMBER] = "NUMBER",
	[TK_FLOATING] = "FLOATING",
	[TK_STRINGLIT] = "STRING",
	[TK_CHARLIT] = "CHAR",
	[TK_INT] = "int",
	[TK_HEX] = "HEX",
	[TK_OCTAL] = "OCTAL",
	[TK_BINARY] = "BINARY",
	[TK_UINT] = "unsigned",
	[TK_FLOAT] = "float",
	[TK_DOUBLE] = "double",
	[TK_BOOL] = "bool",
	[TK_VOID] = "void",
	[TK_CHAR] = "char",
	[TK_STRING] = "string",
	[TK_IF] = "if",
	[TK_ELIF] = "elif",
	[TK_ELSE] = "else",
	[TK_LOOP] = "while",
	[TK_RETURN] = "ret",
	[TK_BREAK] = "break",
	[TK_CONTINUE] = "continue",
	[TK_STRUCT] = "struct",
	[TK_ENUM] = "enum",
	[TK_IMPORT] = "import",
	[TK_TRUE] = "true",
	[TK_FALSE] = "false",
	[TK_LET] = "let",
	[TK_CONST] = "const",
	[TK_FN] = "fn",
	[TK_SIZE_OF] = "sizeof",
	[TK_SYSCALL] = "syscall",
	[TK_FOR] = "for",
	[TK_IN] = "in",
	[TK_MATCH] = "match",
	[TK_DEFER] = "defer",
	[TK_IMPL] = "impl",
	[TK_TYPE] = "type",
	[TK_AS] = "as",
	[TK_NULL] = "null",
	[TK_LPAREN] = "LPAREN",
	[TK_RPAREN] = "RPAREN",
	[TK_LBRACE] = "LBRACE",
	[TK_RBRACE] = "RBRACE",
	[TK_LBRACKET] = "LBRACKET",
	[TK_RBRACKET] = "RBRACKET",
	[TK_SEMICOLON] = "SEMICOLON",
	[TK_COMMA] = "COMMA",
	[TK_DOT] = "DOT",
	[TK_EQUAL] = "EQUAL",
	[TK_PLUS] = "PLUS",
	[TK_MINUS] = "MINUS",
	[TK_STAR] = "STAR",
	[TK_SLASH] = "SLASH",
	[TK_LT] = "LT",
	[TK_GT] = "GT",
	[TK_LE] = "LE",
	[TK_GE] = "GE",
	[TK_CMP] = "CMP",
	[TK_NEQ] = "NEQ",
	[TK_AMP] = "AMP",
	[TK_PIPE] = "PIPE",
	[TK_CARET] = "CARET",
	[TK_TILDE] = "TILDE",
	[TK_AND] = "AND",
	[TK_OR] = "OR",
	[TK_COLON] = "COLON",
	[TK_NOT] = "NOT",
	[TK_QUESTION] = "QUESTION",
	[TK_INCREMENT] = "INCREMENT",
	[TK_DECREMENT] = "DECREMENT",
	[TK_LSHIFT] = "LSHIFT",
	[TK_RSHIFT] = "RSHIFT",
	[TK_LARROW] = "LARROW",
	[TK_RARROW] = "RARROW",
	[TK_MOD] = "MOD",
	[TK_NAMESPACE] = "NAMESPACE",
	[TK_RANGE] = "RANGE",
	[TK_FATARROW] = "FATARROW",
	[TK_SPREAD] = "SPREAD",
	[TK_PLUSEQ] = "PLUSEQ",
	[TK_MINUSEQ] = "MINUSEQ",
	[TK_STAREQ] = "STAREQ",
	[TK_SLASHEQ] = "SLASHEQ",
	[TK_MODEQ] = "MODEQ",
	[TK_UNDERSCORE] = "UNDERSCORE",
};

const char *token_type_str(enum token_type type)
{
	if ((unsigned)type < ARRAY_SIZE(token_type_names) && token_type_names[type])
		return token_type_names[type];
	return "UNKNOWN";
}

const char *token_type_pretty(enum token_type type)
{
	switch (type) {
	case TK_SEMICOLON:
		return ";";
	case TK_COMMA:
		return ",";
	case TK_COLON:
		return ":";
	case TK_DOT:
		return ".";
	case TK_LPAREN:
		return "(";
	case TK_RPAREN:
		return ")";
	case TK_LBRACE:
		return "{";
	case TK_RBRACE:
		return "}";
	case TK_LBRACKET:
		return "[";
	case TK_RBRACKET:
		return "]";
	case TK_EQUAL:
		return "=";
	case TK_RARROW:
		return "->";
	case TK_FATARROW:
		return "=>";
	case TK_RANGE:
		return "..";
	case TK_NAMESPACE:
		return "::";
	case TK_EOF:
		return "end of file";
	default:
		return token_type_str(type);
	}
}

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

static struct token error_token(struct lexer_context *lexer, const char *start,
				int len, int line, int col, const char *msg)
{
	struct source_location loc = {
		.file = lexer->file,
		.line_start = start - col,
		.line = line,
		.col = col,
		.len = len,
	};

	diag_emit(lexer->diag, ERROR, loc, "%s", msg);
	return (struct token){ TK_ERROR, start, len, line, col };
}

static void try_ignore_comments(struct lexer_context *lexer)
{
	if (is_next(lexer, '/')) {
		advance(lexer);
		while (!is_end(lexer) && !is_next(lexer, '\n'))
			advance(lexer);
		return;
	} else if (is_next(lexer, '*')) {
		int start_line = lexer->line;
		int start_col = lexer->col - 1;
		const char *start = lexer->current - 1;

		advance(lexer);
		while (!is_end(lexer)) {
			if (follow_next(lexer, '*') && follow_next(lexer, '/'))
				return;
			advance(lexer);
		}
		error_token(lexer, start, 2, start_line, start_col, "unterminated block comment");
		return;
	}
	return;
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
			if (lexer->current[1] == '/' || lexer->current[1] == '*') {
				advance(lexer);
				try_ignore_comments(lexer);
			} else {
				return;
			}
			break;
		default:
			return;
		}
	}
}

static struct token token_id(struct lexer_context *lexer, const char *start)
{
	size_t len;

	while (!is_end(lexer) && (isalnum(*lexer->current) || is_next(lexer, '_')))
		advance(lexer);

	len = lexer->current - start;

	/*
	 * Keywords are known at compile time and are fixed.
	 * Dispatch on first character, filter by length, memcmp the rest.
	 *
	 * If someday the number of keywords gets increased, cases should
	 * have anothere switch dispatching on len.
	 */
	switch (start[0]) {
	case 'a':
		if (len == 2 && !memcmp(start, "as", 2))
			return create_token(TK_AS, start, len);
		break;
	case 'b':
		if (len == 4 && !memcmp(start, "bool", 4))
			return create_token(TK_BOOL, start, len);
		if (len == 5 && !memcmp(start, "break", 5))
			return create_token(TK_BREAK, start, len);
		break;
	case 'c':
		if (len == 4 && !memcmp(start, "char", 4))
			return create_token(TK_CHAR, start, len);
		if (len == 5 && !memcmp(start, "const", 5))
			return create_token(TK_CONST, start, len);
		if (len == 8 && !memcmp(start, "continue", 8))
			return create_token(TK_CONTINUE, start, len);
		break;
	case 'd':
		if (len == 5 && !memcmp(start, "defer", 5))
			return create_token(TK_DEFER, start, len);
		if (len == 6 && !memcmp(start, "double", 6))
			return create_token(TK_DOUBLE, start, len);
		break;
	case 'e':
		if (len == 4 && !memcmp(start, "elif", 4))
			return create_token(TK_ELIF, start, len);
		if (len == 4 && !memcmp(start, "else", 4))
			return create_token(TK_ELSE, start, len);
		if (len == 4 && !memcmp(start, "enum", 4))
			return create_token(TK_ENUM, start, len);
		break;
	case 'f':
		if (len == 2 && !memcmp(start, "fn", 2))
			return create_token(TK_FN, start, len);
		if (len == 3 && !memcmp(start, "for", 3))
			return create_token(TK_FOR, start, len);
		if (len == 5 && !memcmp(start, "false", 5))
			return create_token(TK_FALSE, start, len);
		if (len == 5 && !memcmp(start, "float", 5))
			return create_token(TK_FLOAT, start, len);
		break;
	case 'i':
		if (len == 2 && !memcmp(start, "if", 2))
			return create_token(TK_IF, start, len);
		if (len == 2 && !memcmp(start, "in", 2))
			return create_token(TK_IN, start, len);
		if (len == 3 && !memcmp(start, "int", 3))
			return create_token(TK_INT, start, len);
		if (len == 4 && !memcmp(start, "impl", 4))
			return create_token(TK_IMPL, start, len);
		if (len == 6 && !memcmp(start, "import", 6))
			return create_token(TK_IMPORT, start, len);
		break;
	case 'l':
		if (len == 3 && !memcmp(start, "let", 3))
			return create_token(TK_LET, start, len);
		break;
	case 'm':
		if (len == 5 && !memcmp(start, "match", 5))
			return create_token(TK_MATCH, start, len);
		break;
	case 'n':
		if (len == 4 && !memcmp(start, "null", 4))
			return create_token(TK_NULL, start, len);
		break;
	case 'r':
		if (len == 3 && !memcmp(start, "ret", 3))
			return create_token(TK_RETURN, start, len);
		break;
	case 's':
		if (len == 6 && !memcmp(start, "sizeof", 6))
			return create_token(TK_SIZE_OF, start, len);
		if (len == 6 && !memcmp(start, "string", 6))
			return create_token(TK_STRING, start, len);
		if (len == 6 && !memcmp(start, "struct", 6))
			return create_token(TK_STRUCT, start, len);
		if (len == 7 && !memcmp(start, "syscall", 7))
			return create_token(TK_SYSCALL, start, len);
		break;
	case 't':
		if (len == 4 && !memcmp(start, "true", 4))
			return create_token(TK_TRUE, start, len);
		if (len == 4 && !memcmp(start, "type", 4))
			return create_token(TK_TYPE, start, len);
		break;
	case 'u':
		if (len == 8 && !memcmp(start, "unsigned", 8))
			return create_token(TK_UINT, start, len);
		break;
	case 'v':
		if (len == 4 && !memcmp(start, "void", 4))
			return create_token(TK_VOID, start, len);
		break;
	case 'w':
		if (len == 5 && !memcmp(start, "while", 5))
			return create_token(TK_LOOP, start, len);
		break;
	}

	return create_token(TK_ID, start, len);
}

static struct token token_number(struct lexer_context *lexer, const char *start)
{
	if (*start == '0' && !is_end(lexer)) {
		if (follow_next(lexer, 'x') || follow_next(lexer, 'X')) {
			const char *digits = lexer->current;
			while (!is_end(lexer) && isxdigit(*lexer->current))
				advance(lexer);
			if (lexer->current == digits)
				goto error_post_prefix;
			return create_token(TK_HEX, start,
					    (size_t)(lexer->current - start));
		}
		if (follow_next(lexer, 'o') || follow_next(lexer, 'O')) {
			const char *digits = lexer->current;
			while (!is_end(lexer) &&
			       *lexer->current >= '0' && *lexer->current <= '7')
				advance(lexer);
			if (lexer->current == digits)
				goto error_post_prefix;
			return create_token(TK_OCTAL, start,
					    (size_t)(lexer->current - start));
		}
		if (follow_next(lexer, 'b') || follow_next(lexer, 'B')) {
			const char *digits = lexer->current;
			while (!is_end(lexer) &&
			       (*lexer->current == '0' || *lexer->current == '1'))
				advance(lexer);
			if (lexer->current == digits)
				goto error_post_prefix;
			return create_token(TK_BINARY, start,
					    (size_t)(lexer->current - start));
		}
	}

	while (!is_end(lexer) && isdigit(*lexer->current))
		advance(lexer);

	if (!is_end(lexer) && is_next(lexer, '.') && lexer->current[1] != '.') {
		advance(lexer);
		while (!is_end(lexer) && isdigit(*lexer->current))
			advance(lexer);
		return create_token(TK_FLOATING, start, (size_t)(lexer->current - start));
	}

	return create_token(TK_NUMBER, start, (size_t)(lexer->current - start));

error_post_prefix:

	return error_token(lexer, start, (int)(lexer->current - start),
			   lexer->line, lexer->col - 2,
			   "expected digits after the prefix");
}

static const char *check_valid_escape(struct lexer_context *lexer, char esc)
{
	switch (*lexer->current) {
	case 'n':
	case 't':
	case 'r':
	case '\\':
	case '\'':
	case '"':
	case '0':
		advance(lexer);
		return NULL;
	case 'x':
		advance(lexer);
		if (is_end(lexer) || !isxdigit(*lexer->current)) {
			while (!is_end(lexer) && !is_next(lexer, esc))
				advance(lexer);
			advance(lexer);
			return "expected hex digit after '\\x'";
		}
		advance(lexer);
		if (!is_end(lexer) && isxdigit(*lexer->current))
			advance(lexer);
		return NULL;
	default:
		while (!is_end(lexer) && !is_next(lexer, esc))
			advance(lexer);
		advance(lexer);
		return "invalid escape sequence";
	}
}

static struct token token_stringlit(struct lexer_context *lexer, const char *start)
{
	int start_line = lexer->line;
	int start_col = lexer->col - 1;

	while (!is_end(lexer) && !is_next(lexer, '"')) {
		/*
		 * Ignore what's after '\' to avoid early returns e.g.:
		 * "\"Hello\""
		 */
		if (is_next(lexer, '\\')) {
			advance(lexer);
			const char *err = check_valid_escape(lexer, '"');
			if (err)
				return error_token(lexer, start,
						   (int)(lexer->current - start),
						   start_line, start_col, err);
		} else {
			advance(lexer);
		}
	}

	if (is_end(lexer))
		return error_token(lexer, start, (int)(lexer->current - start),
				   start_line, start_col, "unterminated string");

	advance(lexer);

	return create_token(TK_STRINGLIT, start, (size_t)(lexer->current - start));
}

static struct token token_charlit(struct lexer_context *lexer, const char *start)
{
	int start_line = lexer->line;
	int start_col = lexer->col - 1;

	if (is_next(lexer, '\''))
		return error_token(lexer, start, 1, start_line, start_col, "empty char literal");

	if (is_next(lexer, '\\')) {
		advance(lexer);
		const char *err = check_valid_escape(lexer, '\'');
		if (err)
			return error_token(lexer, start,
					   (int)(lexer->current - start),
					   start_line, start_col, err);
	} else {
		advance(lexer);
	}

	if (!is_next(lexer, '\''))
		return error_token(lexer, start,
				   (int)(lexer->current - start), start_line,
				   start_col, "unterminated char literal");

	advance(lexer);

	return create_token(TK_CHARLIT, start, (size_t)(lexer->current - start));
}

void lexer_init(struct lexer_context *lexer, struct compiler_context *cc)
{
	lexer->src = cc->src;
	lexer->current = cc->src;
	lexer->diag = &cc->diag;
	lexer->file = cc->filename;
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
		if (isalnum(*lexer->current) || *lexer->current == '_')
			return token_id(lexer, start);
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
		return error_token(lexer, start, 1, lexer->line, lexer->col - 1,
				   "unexpected character");
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
		printf("%.*s [%s] - %d:%d\n", (int)token.len, token.lex,
		       token_type_str(token.type),
		       token.line, token.col);
	}

	return errors;
}
