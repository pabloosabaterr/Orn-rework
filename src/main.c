#include "lexer/lexer.h"
#include "memory/wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct compiler_context {
	struct lexer_context lexer;
	unsigned dump_tokens:1;
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
	char *src = NULL;
	int i;

	if (argc < 2)
		die("empty input");

	ctx->expect_file = 1;
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--dump-tokens"))
			ctx->dump_tokens = 1;
		else if (ctx->expect_file) {
			ctx->expect_file = 0;
			src = read_file(argv[i]);
		}
	}

	if (!src)
		die("no input file provided");

	lexer_init(&ctx->lexer, src);

	if (ctx->dump_tokens) {
		int errors = dump_tokens(&ctx->lexer);
		free(src);
		free(ctx);
		return errors ? 1 : 0;
	}

	free(src);
	free(ctx);
	return 0;
}
