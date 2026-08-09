#include "compiler.h"
#include "diagnostic.h"
#include "lexer.h"
#include "parse-options.h"
#include "wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path)
{
    FILE *f;
    long len;
    char *buf;
    size_t n;

    f = fopen(path, "r");
    if (!f)
        die("cannot open '%s'", path);

    if (fseek(f, 0, SEEK_END))
        die("cannot seek '%s'", path);
    len = ftell(f);
    if (len < 0)
        die("cannot know size of '%s'", path);
    if (fseek(f, 0, SEEK_SET))
        die("cannot seek '%s'", path);

    buf = xmalloc(len + 1);
    n = fread(buf, 1, len, f);
    buf[n] = '\0';
    fclose(f);

    return buf;
}

int main(int argc, char **argv)
{
    struct compiler_context cc = COMPILER_CONTEXT_INIT;
    const char *filename = NULL;
    struct lexer_context lexer;
    char *src = NULL;
    int opts = -1;
    int ret = 0;

    struct option options[] = {
        OPT_BOOL('t', "dump-tokens", &cc.dump_tokens),
        OPT_END()
    };

    opts = parse_options(argc, argv, options);

    if (opts < argc)
        filename = argv[opts];

    if (!filename)
        die("no input file provided");

    src = read_file(filename);

    compiler_init(&cc, filename, src);
    lexer_init(&lexer, &cc);

    dump_tokens(&lexer);

    diag_flush(&cc.diag, stderr);
    printf("Program compiled with %d %s\n", cc.diag.nr_error,
           cc.diag.nr_error == 1 ? "error" : "errors");

    ret = diag_has_errors(&cc.diag) ? 1 : 0;

    free(src);
    compiler_free(&cc);
    return ret;
}
