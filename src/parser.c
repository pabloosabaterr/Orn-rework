#include "parser.h"
#include "arena.h"
#include "attrs.h"
#include "compiler.h"
#include "diagnostic.h"
#include "arena.h"
#include "lexer.h"
#include "log.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static struct token advance_token(struct parser_context *p)
{
    if (p->current.type == TK_UNINIT)
        p->current = lexer_get_next_token(p->lexer);

    p->prev = p->current;
    p->current = lexer_get_next_token(p->lexer);
    return p->prev;
}

static int check_token(struct parser_context *p, enum token_type type)
{
    return p->current.type == type;
}

static int check_and_advance(struct parser_context *p, enum token_type type)
{
    if (!check_token(p, type))
        return 0;
    advance_token(p);
    return 1;
}

static struct source_location loc_from_token(struct parser_context *p, struct token tok)
{
    return (struct source_location){
        .file = p->file,
        .line_start = tok.lex - tok.col,
        .line = tok.line,
        .col = tok.col,
        .len = (int)tok.len,
    };
}

UNUSED
static void expect_token(struct parser_context *p, enum token_type type)
{
    if (!check_and_advance(p, type) && !p->in_panic) {
        diag_emit(p->diag, ERROR, loc_from_token(p, p->prev), "expected '%s' after '%.*s'",
                  lexer_get_token_pretty(type), (int)p->prev.len, p->prev.lex);
        p->in_panic = 1;
    }
}

UNUSED
static struct parser_ast_node *create_node(struct parser_context *p, enum node_type type)
{
   struct parser_ast_node *node = arena_alloc(p->arena, sizeof(struct parser_ast_node));
   memset(node, 0, sizeof(struct parser_ast_node));
   node->tok = p->current;
   node->type = type;

   return node;
}

/*
 * Returns 1 on failing to append.
 * Realloc's if the block list is full.
 */
static int append_node_to_block(struct parser_context *p,
                                struct parser_ast_node *block,
                                struct parser_ast_node *to_append)
{
    if (block->type != NODE_PROGRAM && block->type != NODE_BLOCK)
        return 1;

    if (block->block.nr >= block->block.alloc)
        ARENA_ALLOC_GROW(p->arena, block->block.childs, block->block.nr + 1, block->block.alloc);

    block->block.childs[block->block.nr++] = to_append;

    return 0;
}

/*
 * stmt = binding SEMI
 *      | label
 *      | GOTO label SEMI
 *      | loop_stmt
 *      | RET expr? SEMI
 *      | BREAK SEMI
 *      | CONTINUE SEMI
 *      | expr SEMI
 */
static struct parser_ast_node *parse_stmt(struct parser_context *p UNUSED)
{
    die("not done");
}

/*
 * NODE_PROGRAM is just a cool block
 * program = stmt*
 */
static struct parser_ast_node *parse_program(struct parser_context *p)
{
    advance_token(p);

    struct parser_ast_node *program = create_node(p, NODE_PROGRAM);
    while(!check_token(p, TK_EOF)) {
        struct parser_ast_node *stmt = parse_stmt(p);

        if (stmt)
            append_node_to_block(p, program, stmt);

    }

    return program;
}

struct parser_ast_node *parser_parse(struct parser_context *p)
{
    return parse_program(p);
}

void parser_print(struct parser_ast_node *program)
{
    assert(program);
    printf("PROGRAM\n");
}

void parser_init(struct parser_context *p, struct lexer_context *lexer, struct compiler_context *cc)
{
    struct token t = TOKEN_INIT;

    p->lexer = lexer;
    p->file = cc->filename;
    p->diag = &cc->diag;
    p->arena = &cc->arena;
    p->current = t;
}
