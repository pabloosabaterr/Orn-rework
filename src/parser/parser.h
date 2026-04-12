#ifndef PARSER_H
#define PARSER_H

#include "lexer/lexer.h"
#include "memory/arena.h"

/* 16KB ~ 204 nodes */
#define PARSER_ARENA_DEF (16384)

struct parser_context {
	struct lexer_context *lexer;
	struct token current;
	struct token prev;
	struct arena arena;
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
};

void parser_init(struct parser_context *ctx, struct lexer_context *lexer);
struct ast_node *parser_parse(struct parser_context *parser);
void parser_free(struct parser_context *parser);

#endif
