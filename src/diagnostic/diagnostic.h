#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include <stddef.h>
#include <stdio.h>

enum severity_level {
	NOTE,
	WARNING,
	ERROR,
};

struct source_location {
	const char *file;
	const char* line_start;
	int line;
	int col;
	int len;
};

struct diagnostic {
	enum severity_level level;
	struct source_location loc;
	char *msg;
};

struct diag_context {
	struct diagnostic *diags;
	size_t nr;
	size_t alloc;
	int nr_error;
	int nr_warning;
};

void diag_init(struct diag_context *ctx);
void diag_free(struct diag_context *ctx);

void diag_emit(struct diag_context *ctx, enum severity_level level,
	       struct source_location loc, const char *fmt, ...);

int diag_has_errors(struct diag_context *ctx);

void diag_flush(struct diag_context *ctx, FILE *out);

#endif
