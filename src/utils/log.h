#ifndef LOG_H
#define LOG_H

#include "utils/attrs.h"

NORETURN void die(const char *fmt, ...) PRINTF_FMT(1, 2);
NORETURN void BUG(const char *fmt, ...) PRINTF_FMT(1, 2);

#endif /* LOG_H */
