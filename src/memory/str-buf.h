#ifndef STR_BUF_H
#define STR_BUF_H

#include "utils/attrs.h"
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>

#define STR_BUF_INIT {NULL, 0, 0}

struct str_buf {
	char *buf;
	size_t size;
	size_t len;
};

void str_buf_vaddf(struct str_buf *sb, const char *fmt, va_list ap)
	PRINTF_FMT(2, 0);
void str_buf_addf(struct str_buf *sb, const char *fmt, ...)
	PRINTF_FMT(2, 3);
void str_buf_addstr(struct str_buf *sb, const char *s);
char *str_buf_detach(struct str_buf *sb);
void str_buf_release(struct str_buf *sb);

#endif /* STR_BUF_H */
