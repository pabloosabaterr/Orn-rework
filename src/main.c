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

static int orn_cmd_parse(int argc, char**argv, struct compiler_context *cc)
{
    int opts = -1;

    struct option options[] = {
        OPT_BOOL('t', "dump-tokens", &cc->dump_tokens),
        OPT_BOOL('a', "dump-ast", &cc->dump_ast),
        OPT_END()
    };

    opts = parse_options(argc, argv, options);

    if (opts < argc)
        cc->filename = argv[opts];

    if (!cc->filename)
        die("no input file provided");

    return opts;
}

int main(int argc, char **argv)
{
    struct compiler_context cc = COMPILER_CONTEXT_INIT;
    struct parser_context parser = PARSER_CONTEXT_INIT;
    struct lexer_context lexer = LEXER_CONTEXT_INIT;
    struct parser_ast_node *program = NULL;
    char *src = NULL;
    int ret = 0;

    orn_cmd_parse(argc, argv, &cc);

    src = read_file(cc.filename);

    compiler_init(&cc, src);
    lexer_init(&lexer, &cc);
    parser_init(&parser, &lexer, &cc);

    if (cc.dump_tokens) {
        lexer_dump_tokens(&lexer);
        goto cleanup;
    }

    program = parser_parse(&parser);

    /*
     * Even thought printing the AST does not consume the nodes and the compilation could keep going
     * because printing the tokens cannot keep with the compilation, let's be consistent and stop
     * it also for the ast printing.
     */
    if (cc.dump_ast) {
        parser_print(program);
        goto cleanup;
    }

    diag_flush(&cc.diag, stderr);
    printf("Program compiled with %d %s\n", cc.diag.nr_error,
           cc.diag.nr_error == 1 ? "error" : "errors");

    ret = diag_has_errors(&cc.diag) ? 1 : 0;

cleanup:
    free(src);
    compiler_free(&cc);
    return ret;
}
