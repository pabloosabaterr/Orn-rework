#ifndef WRAPPER_H
#define WRAPPER_H

#include <stddef.h>
#include <stdint.h>
#include "utils/log.h"

static inline int check_mult_overflow(size_t a, size_t b)
{
	return a && b && a > SIZE_MAX / b;
}

static inline size_t safe_mult(size_t a, size_t b)
{
	if (check_mult_overflow(a, b))
		die("size_t overflow: %zu * %zu", a, b);
	return a * b;
}

static inline size_t safe_add(size_t a, size_t b)
{
	if (a > SIZE_MAX - b)
		die("size_t overflow: %zu + %zu", a, b);
	return a + b;
}

#define alloc_needed(x) (((x)+32)*3/2)

#define FREE_AND_NULL(p) do { free(p); (p) = NULL; } while (0)

#define ALLOC_ARRAY(x, to_alloc) (x) = xmalloc(safe_mult(sizeof(*(x)), (to_alloc)))
#define CALLOC_ARRAY(x, to_alloc) (x) = xcalloc((to_alloc), sizeof(*(x)))
#define REALLOC_ARRAY(x, new_size) (x) = xrealloc((x), safe_mult(sizeof(*(x)), (new_size)))
#define MEMZERO_ARRAY(x, to_zero) memset((x), 0x0, safe_mult(sizeof(*(x)), (to_zero)))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/*
 * Grow an array dynamically.
 *
 * count is how many would be after the growth.
 * e.g:
 *
 * if you had 5 and wanted to add 3, count would be 8.
 *
 * Use:
 * ptr, count, allocated = {0};
 * ALLOC_GROW(ptr, count+1, allocated);
 * ptr[count++] = new_element;
 * Caller is responsible for updating the `count` variable.
 *
 * Dont use expressions with side effects.
 */
#define ALLOC_GROW(ptr, count, allocated) \
	do { \
		if ((count) > (allocated)) { \
			if (alloc_needed(allocated) < (count)) \
				allocated  = (count); \
			else \
				allocated = alloc_needed(allocated); \
			REALLOC_ARRAY(ptr, allocated); \
		} \
	} while (0)

void *xmalloc(size_t size);
void *xcalloc(size_t num, size_t size);
char *xstrdup(const char *s);
void *xrealloc(void *ptr, size_t size);
long long xstrtoll(const char *s, size_t len, int base);
double xstrtod(const char *s, size_t len);

#endif /* WRAPPER_H */
