#include "compiler.h"
#include "diagnostic.h"
#include "io.h"
#include "lexer.h"
#include "log.h"
#include "parse-options.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct compiler_context cc = COMPILER_CONTEXT_INIT;
    struct parser_context parser = PARSER_CONTEXT_INIT;
    struct lexer_context lexer = LEXER_CONTEXT_INIT;
    const char *filename = NULL;
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
    parser_init(&parser, &lexer, &cc);

    if (cc.dump_tokens)
        dump_tokens(&lexer);
    diag_flush(&cc.diag, stderr);
    printf("Program compiled with %d %s\n", cc.diag.nr_error,
           cc.diag.nr_error == 1 ? "error" : "errors");

    ret = diag_has_errors(&cc.diag) ? 1 : 0;

    free(src);
    compiler_free(&cc);
    return ret;
}
