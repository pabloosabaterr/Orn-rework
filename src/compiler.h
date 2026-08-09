#ifndef COMPILER_H
#define COMPILER_H

#include "diagnostic.h"
#include "arena.h"

struct compiler_options {
    unsigned dump_tokens : 1;
};

#define COMPILER_OPTIONS_INIT { 0 }

struct compiler_context {
    struct arena arena;
    struct diag_context diag;

    const char *filename;
    const char *src;

    struct compiler_options options;
};

#define COMPILER_CONTEXT_INIT { 0 }

void compiler_init(struct compiler_context *cc, const char *filename, const char *src,
                   struct compiler_options options);
void compiler_free(struct compiler_context *cc);

#endif
