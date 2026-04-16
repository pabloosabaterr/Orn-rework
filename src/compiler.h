#ifndef COMPILER_H
#define COMPILER_H

#include "diagnostic/diagnostic.h"
#include "memory/arena.h"

struct compiler_options {
	unsigned dump_tokens:1;
	unsigned dump_ast:1;
};

struct compiler_context {
	struct arena arena;
	struct diag_context diag;

	const char *filename;
	const char *src;

	struct compiler_options options;
};


void compiler_init(struct compiler_context *cc, const char *filename,
		   const char *src, struct compiler_options options);
void compiler_free(struct compiler_context *cc);

#endif
