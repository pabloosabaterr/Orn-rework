#include "compiler.h"

/* 64kb initial */
#define COMPILER_ARENA_DEF (64 * 1024)

void compiler_init(struct compiler_context *cc, const char *filename,
		   const char *src, struct compiler_options options)
{
	arena_init(&cc->arena, COMPILER_ARENA_DEF);
	diag_init(&cc->diag);
	cc->filename = filename;
	cc->src = src;
	cc->options = options;
}

void compiler_free(struct compiler_context *cc)
{
	arena_free(&cc->arena);
	diag_free(&cc->diag);
	cc->filename = NULL;
	cc->src = NULL;
}
