#ifndef PARSER_H
#define PARSER_H

#include "diagnostic/diagnostic.h"
#include "lexer/lexer.h"

struct compiler_context;
struct diag_context;
struct arena;

struct parser_context {
	struct lexer_context *lexer;
	struct diag_context *diag;
	const char* file;
	struct token current;
	struct token prev;
	struct arena *arena;
	int errors;
	/*
	 * If statements doesn't use parentheses so it is ambiguous whether if
	 * it's a if or a struct init.
	 *
	 * if x {...}
	 *
	 * and
	 *
	 * x {...}
	 *
	 * The flag allows a struct init to be parsed.
	 */
	unsigned no_struct_init:1;
	unsigned in_panic:1;
};

void parser_init(struct parser_context *ctx, struct lexer_context *lexer,
		 struct compiler_context *cc);
struct ast_node *parser_parse(struct parser_context *parser);
void parser_free(struct parser_context *parser);

#endif
