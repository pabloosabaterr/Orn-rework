#ifndef STR_BUF_H
#define STR_BUF_H

#include <stddef.h>

#define STR_BUF_INIT {NULL, 0, 0}

struct str_buf {
	char *buf;
	size_t size;
	size_t len;
};

void str_buf_addf(struct str_buf *sb, const char *fmt, ...);
void str_buf_addstr(struct str_buf *sb, const char *s);
char *str_buf_detach(struct str_buf *sb);
void str_buf_release(struct str_buf *sb);

#endif /* STR_BUF_H */
