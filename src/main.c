#include "diagnostic/diagnostic.h"
#include "lexer/lexer.h"
#include "parser/ast.h"
#include "parser/parser.h"
#include "memory/wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct compiler_context {
	struct lexer_context lexer;
	struct diag_context diag;
	unsigned dump_tokens:1;
	unsigned dump_ast:1;
	unsigned expect_file:1;
};

static char *read_file(const char *path)
{
	FILE *f;
	long len;
	char *buf;
	size_t n;

	f = fopen(path, "r");
	if (!f)
		die("cannot open '%s'", path);

	if (fseek(f, 0, SEEK_END))
		die("cannot seek '%s'", path);
	len = ftell(f);
	if (len < 0)
		die("cannot know size of '%s'", path);
	if (fseek(f, 0, SEEK_SET))
		die("cannot seek '%s'", path);

	buf = xmalloc(len + 1);
	n = fread(buf, 1, len, f);
	buf[n] = '\0';
	fclose(f);

	return buf;
}

int main(int argc, char **argv)
{
	struct compiler_context *ctx = xcalloc(1, sizeof(*ctx));
	struct parser_context parser;
	char *src = NULL, *filename = NULL;
	int i, ret;

	if (argc < 2)
		die("empty input");

	ctx->expect_file = 1;
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--dump-tokens"))
			ctx->dump_tokens = 1;
		else if (!strcmp(argv[i], "--dump-ast"))
			ctx->dump_ast = 1;
		else if (ctx->expect_file) {
			ctx->expect_file = 0;
			filename = argv[i];
			src = read_file(argv[i]);
		}
	}

	if (!src)
		die("no input file provided");
	/*
	 * Dumping the token eats the tokens in the proccess
	 * and AST dumping need those tokens.
	 * RFC: Duplicating the lexer context to be able to dump both or keep
	 * the address of the first token to reset the lexer after dumping it.
	 */
	if (ctx->dump_ast && ctx->dump_tokens)
		die("--dump-ast and --dump-tokens cannot live together");

	diag_init(&ctx->diag);
	lexer_init(&ctx->lexer, src, filename, &ctx->diag);

	if (ctx->dump_tokens) {
		dump_tokens(&ctx->lexer);
		diag_flush(&ctx->diag, stderr);
		ret = diag_has_errors(&ctx->diag);
	}

	/* initializing the parser "steals" the first token */
	parser_init(&parser, &ctx->lexer, filename, &ctx->diag);

	if (ctx->dump_ast) {
		struct ast_node *program = parser_parse(&parser);
		ast_dump(program);
		diag_flush(&ctx->diag, stderr);
		ret = diag_has_errors(&ctx->diag);
	}

	printf("Program compiled with %d %s\n", ctx->diag.nr_error,
	       ctx->diag.nr_error == 1 ? "error" : "errors");

	free(src);
	diag_free(&ctx->diag);
	free(ctx);
	parser_free(&parser);
	return ret ? 1 : 0;
}
