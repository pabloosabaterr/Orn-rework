#include "diagnostic/diagnostic.h"
#include "memory/str-buf.h"
#include "memory/wrapper.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void diag_init(struct diag_context *d)
{
	memset(d, 0, sizeof(*d));
}

void diag_free(struct diag_context *d)
{
	size_t i;

	for (i = 0; i < d->nr; i++)
		free(d->diags[i].msg);
	free(d->diags);
	memset(d, 0, sizeof(*d));
}

void diag_emit(struct diag_context *d, enum severity_level level,
	       struct source_location loc, const char *fmt, ...)
{
	struct str_buf sb = STR_BUF_INIT;
	va_list ap;

	va_start(ap, fmt);
	str_buf_vaddf(&sb, fmt, ap);
	va_end(ap);

	ALLOC_GROW(d->diags, d->nr + 1, d->alloc);
	d->diags[d->nr].level = level;
	d->diags[d->nr].loc = loc;
	d->diags[d->nr].msg = str_buf_detach(&sb);
	d->nr++;

	if (level == ERROR)
		d->nr_error++;
	else if (level == WARNING)
		d->nr_warning++;
}

static void diag_render(struct diagnostic *d, FILE *f)
{
	static const char *level_str[] = {
		[NOTE] = "note",
		[WARNING] = "warning",
		[ERROR] = "error",
	};

	const char *end, *c;
	int padding, i;

	fprintf(f, "%s: %s\n", level_str[d->level], d->msg);
	fprintf(f, " --> %s:%d:%d\n", d->loc.file, d->loc.line, d->loc.col);

	end = d->loc.line_start;
	while (*end && *end != '\n')
		end++;

	padding = snprintf(NULL, 0, "%d", d->loc.line);
	fprintf(f, " %*s |\n", padding, "");
	fprintf(f, " %d | ", d->loc.line);
	for (c = d->loc.line_start; c < end; c++)
		if (*c == '\t')
			fprintf(f, "    ");
		else
			fputc(*c, f);
	fputc('\n', f);

	fprintf(f, " %*s | ", padding, "");
	for (i = 0; i < d->loc.col; i++)
		if (d->loc.line_start[i] == '\t')
			fprintf(f, "    ");
		else
			fputc(' ', f);
	for (i = 0; i < (d->loc.len > 0 ? d->loc.len : 1); i++)
		fputc(i == 0 ? '^' : '~', f);
	fputc('\n', f);
}

int diag_has_errors(struct diag_context *d)
{
	return d->nr_error > 0;
}

void diag_flush(struct diag_context *d, FILE *f)
{
	size_t i;

	for (i = 0; i < d->nr; i++)
		diag_render(&d->diags[i], f);
}
