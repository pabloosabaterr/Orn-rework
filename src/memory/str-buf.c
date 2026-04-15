#include "memory/str-buf.h"
#include "memory/wrapper.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void str_buf_vaddf(struct str_buf *sb, const char *fmt, va_list ap)
{
	va_list cp;
	int n;

	va_copy(cp, ap);
	n = vsnprintf(NULL, 0, fmt, cp);
	va_end(cp);

	if (n < 0)
		die("str_buf_vaddf: vsnprintf failed");

	ALLOC_GROW(sb->buf, sb->len + n + 1, sb->size);
	vsnprintf(sb->buf + sb->len, n + 1, fmt, ap);
	sb->len += n;
}

void str_buf_addf(struct str_buf *sb, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	str_buf_vaddf(sb, fmt, ap);
	va_end(ap);
}

void str_buf_addstr(struct str_buf *sb, const char *s)
{
	size_t len = strlen(s);
	ALLOC_GROW(sb->buf, sb->len + len + 1, sb->size);
	memcpy(sb->buf + sb->len, s, len);
	sb->len += len;
	sb->buf[sb->len] = '\0';
}

char *str_buf_detach(struct str_buf *sb)
{
	char *buf = sb->buf;

	*sb = (struct str_buf)STR_BUF_INIT;
	return buf;
}

void str_buf_release(struct str_buf *sb)
{
	free(sb->buf);
	*sb = (struct str_buf)STR_BUF_INIT;
}
