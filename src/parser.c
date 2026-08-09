#include "parser.h"
#include "compiler.h"
#include "diagnostic.h"
#include "lexer.h"
#include "arena.h"
#include "wrapper.h"
#include "attrs.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

static struct token advance(struct parser_context *p)
{
    p->prev = p->current;
    p->current = token_next(p->lexer);
    return p->prev;
}

static int check(struct parser_context *p, enum token_type type)
{
    return p->current.type == type;
}

static int match(struct parser_context *p, enum token_type type)
{
    if (!check(p, type))
        return 0;
    advance(p);
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
static void expect(struct parser_context *p, enum token_type type)
{
    if (!match(p, type) && !p->in_panic) {
        diag_emit(p->diag, ERROR, loc_from_token(p, p->prev), "expected '%s' after '%.*s'",
                  token_type_pretty(type), (int)p->prev.len, p->prev.lex);
        p->in_panic = 1;
    }
}

void parser_init(struct parser_context *p, struct lexer_context *lexer, struct compiler_context *cc)
{
    memset(p, 0, sizeof(*p));
    p->lexer = lexer;
    p->file = cc->filename;
    p->diag = &cc->diag;
    p->arena = &cc->arena;
    p->current = token_next(lexer);
}
