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
};

void parser_init(struct parser_context *ctx, struct lexer_context *lexer);
struct ast_node *parser_parse_expr(struct parser_context *parser);
void parser_free(struct parser_context *parser);

#endif
