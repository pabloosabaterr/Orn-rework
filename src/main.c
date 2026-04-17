#include "compiler.h"
#include "diagnostic/diagnostic.h"
#include "lexer/lexer.h"
#include "parser/ast.h"
#include "parser/parser.h"
#include "memory/wrapper.h"
#include "semantic/semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * NEEDSWORK: should go to a IO file later when more IO related functions
 * come or when imports and modules come in.
 */
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
	struct compiler_context cc;
	struct compiler_options options = { 0 };
	struct lexer_context lexer;
	struct parser_context parser;
	struct semantic_context sem;
	struct ast_node *program;
	char *src = NULL;
	const char *filename = NULL;
	int i, ret = 0;

	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i], "--dump-tokens"))
			options.dump_tokens = 1;
		else if (!strcmp(argv[i], "--dump-ast"))
			options.dump_ast = 1;
		else if (argv[i][0] == '-')
			die("unknown option '%s'", argv[i]);
		else if (!filename)
			filename = argv[i];
		else
			die("multiple input files not supported");

	if (!filename)
		die("no input file provided");
	/*
	 * Dumping the token eats the tokens in the proccess
	 * and AST dumping need those tokens.
	 * RFC: Duplicating the lexer context to be able to dump both or keep
	 * the address of the first token to reset the lexer after dumping it.
	 */
	if (options.dump_ast && options.dump_tokens)
		die("--dump-ast and --dump-tokens cannot live together");

	src = read_file(filename);

	compiler_init(&cc, filename, src, options);

	lexer_init(&lexer, &cc);

	if (options.dump_tokens) {
		dump_tokens(&lexer);
		goto report;
	}

	/* initializing the parser "steals" the first token */
	parser_init(&parser, &lexer, &cc);
	program = parser_parse(&parser);

	if (options.dump_ast) {
		ast_dump(program);
		goto report;
	}

	sem_init(&sem, &cc);
	semantic_analyze(&sem, program);

report:
	diag_flush(&cc.diag, stderr);
	printf("Program compiled with %d %s\n", cc.diag.nr_error,
	       cc.diag.nr_error == 1 ? "error" : "errors");

	ret = diag_has_errors(&cc.diag) ? 1 : 0;

	if (!options.dump_tokens)
		parser_free(&parser);
	free(src);
	compiler_free(&cc);
	return ret;
}
