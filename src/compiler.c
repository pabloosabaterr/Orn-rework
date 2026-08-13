#include "compiler.h"
#include "io.h"
#include <stdlib.h>
#include <string.h>

/* 64kb initial */
#define COMPILER_ARENA_DEF (64 * 1024)

void compiler_init(struct compiler_context *cc)
{
    arena_init(&cc->arena, COMPILER_ARENA_DEF);
    diag_init(&cc->diag);
}

void compiler_load(struct compiler_context *cc, const char *path)
{
    cc->file.path = path;
    cc->file.src = read_file(path);
    cc->file.len = strlen(cc->file.src);
}

void compiler_free(struct compiler_context *cc)
{
    free(cc->file.src);
    cc->file.src = NULL;
    cc->file.path = NULL;
    cc->file.len = 0;

    arena_free(&cc->arena);
    diag_free(&cc->diag);
}
