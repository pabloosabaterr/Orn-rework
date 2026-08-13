#ifndef COMPILER_H
#define COMPILER_H

#include "diagnostic.h"
#include "arena.h"
#include "source.h"

struct compiler_context {
    struct arena arena;
    struct diag_context diag;

    struct source_file file;

    unsigned dump_tokens;
    unsigned dump_ast;
};

#define COMPILER_CONTEXT_INIT { 0 }

void compiler_init(struct compiler_context *cc);
void compiler_load(struct compiler_context *cc, const char *path);
void compiler_free(struct compiler_context *cc);

#endif
