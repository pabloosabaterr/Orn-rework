#ifndef LOG_H
#define LOG_H

#include <stdnoreturn.h>

noreturn void die(const char *fmt, ...);
noreturn void BUG(const char *fmt, ...);

#endif
