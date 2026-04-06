#define _POSIX_C_SOURCE 200809L

#include "memory/wrapper.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

void die(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "fatal: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(EXIT_FAILURE);
}

void *xmalloc(size_t size)
{
	void *ret = malloc(size);

	if (!ret && !size)
		ret = malloc(1);
	if (!ret)
		die("out of memory, malloc failed");
	return ret;
}

void *xcalloc(size_t num, size_t size)
{
	void *ret;

	if (check_mult_overflow(num, size))
		die("size_t overflow: %zu * %zu", num, size);

	ret = calloc(num, size);
	if (!ret && (!num || !size))
		ret = calloc(1, 1);
	if (!ret)
		die("out of memory, calloc failed");
	return ret;
}

char *xstrdup(const char *s)
{
	char *ret;

	if (!s)
		die("strdup failed: input string is NULL");

	ret = strdup(s);
	if (!ret)
		die("out of memory, strdup failed");
	return ret;
}

void *xrealloc(void *ptr, size_t size)
{
	void *ret = realloc(ptr, size);

	if (!ret && !size)
		ret = realloc(ptr, 1);
	if (!ret)
		die("out of memory, realloc failed");
	return ret;
}
