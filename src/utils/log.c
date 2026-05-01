#include "utils/log.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

noreturn static void rep_msg(const char *prefix, const char *fmt, va_list ap)
{
	fprintf(stderr, "%s: ", prefix);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

/*
 * Report errors that lead to fatally exit, such as memory alloc failure.
 */
noreturn void die(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	rep_msg("fatal", fmt, ap);
	va_end(ap);
}

/*
 * Report violations of contracts or expected non-NULL.
 * Use it for assert-like checks.
 */
noreturn void BUG(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	rep_msg("BUG", fmt, ap);
	va_end(ap);
}
