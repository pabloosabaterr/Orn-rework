#ifndef SOURCE_H
#define SOURCE_H

#include <stddef.h>

struct source_file {
    const char *path;
    char *src;
    size_t len;
};

#endif
