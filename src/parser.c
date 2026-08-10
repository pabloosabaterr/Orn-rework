#include "parser.h"
#include "arena.h"
#include "attrs.h"
#include "compiler.h"
#include "diagnostic.h"
#include "lexer.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

static struct token advance_token(struct parser_context *p)
{
    if (p->current.type == TK_UNINIT)
        p->current = p->getToken(p->lexer);

    p->prev = p->current;
    p->current = p->getToken(p->lexer);
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
                  token_type_pretty(type), (int)p->prev.len, p->prev.lex);
        p->in_panic = 1;
    }
}

UNUSED
static struct ast_node *create_node(struct parser_context *p, enum node_type type)
{
   struct ast_node *node = arena_alloc(p->arena, sizeof(struct ast_node));
   memset(node, 0, sizeof(struct ast_node));
   node->tok = p->current;
   node->type = type;

   return node;
}

void parser_init(struct parser_context *p, struct lexer_context *lexer, struct compiler_context *cc)
{
    struct token t = TOKEN_INIT;

    p->lexer = lexer;
    p->file = cc->filename;
    p->diag = &cc->diag;
    p->arena = &cc->arena;
    p->getToken = token_next;
    p->current = t;
}
