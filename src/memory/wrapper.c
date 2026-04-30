#define _POSIX_C_SOURCE 200809L

#include "memory/wrapper.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

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

long long xstrtoll(const char *s, size_t len, int base)
{
	/*
	 * Max size of a long long is 20 chars / 22 with prefixes
	 */
	char buf[32];
	char *end;
	long long val;

	if (len >= sizeof(buf))
		die("number too long: '%.*s'", (int)len, s);

	memcpy(buf, s, len);
	buf[len] = '\0';

	errno = 0;
	val = strtoll(buf, &end, base);
	if (errno || end == buf || *end != '\0')
		die("invalid number: '%.*s'", (int)len, s);
	return val;
}

double xstrtod(const char *s, size_t len)
{
	/*
	 * Max size of a double is 24 chars (1.7e-308)
	 */
	char buf[64];
	char *end;
	double val;

	if (len >= sizeof(buf))
		die("floating-point number too long: '%.*s'", (int)len, s);

	memcpy(buf, s, len);
	buf[len] = '\0';

	errno = 0;
	val = strtod(buf, &end);
	if (errno || end == buf || *end != '\0')
		die("invalid floating-point number: '%.*s'", (int)len, s);
	return val;
}
