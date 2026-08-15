#include "compiler.h"
#include "lexer.h"
#include "parse-options.h"
#include "parser.h"
#include "orn.h"

static void orn_run_pipeline(struct compiler_context *cc)
{
    struct parser_context parser = PARSER_CONTEXT_INIT;
    struct lexer_context lexer = LEXER_CONTEXT_INIT;
    struct parser_ast_node *program = NULL;

    lexer_init(&lexer, &cc->file, &cc->diag);

    if (cc->dump_tokens) {
        lexer_dump_tokens(&lexer);
        return;
    }

    parser_init(&parser, &lexer, &cc->arena, &cc->diag);
    program = parser_parse(&parser);

    /*
     * Even thought printing the AST does not consume the nodes and the compilation could keep going
     * because printing the tokens cannot keep with the compilation, let's be consistent and stop
     * it also for the ast printing.
     */
    if (cc->dump_ast) {
        parser_print(program, 0);
        return;
    }
}

int orn_cmd_parse(int argc, char**argv, struct compiler_context *cc)
{
    int opts = -1;

    struct option options[] = {
        OPT_BOOL('t', "dump-tokens", &cc->dump_tokens),
        OPT_BOOL('a', "dump-ast", &cc->dump_ast),
        OPT_END()
    };

    opts = parse_options(argc, argv, options);

    if (opts >= argc)
        die("no input file provided");

    compiler_load(cc, argv[opts]);

    orn_run_pipeline(cc);

    diag_flush(&cc->diag, stderr);
    printf("Program compiled with %d %s\n", cc->diag.nr_error,
           cc->diag.nr_error == 1 ? "error" : "errors");

    return diag_has_errors(&cc->diag) ? 1 : 0;
}
