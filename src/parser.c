#include "parser.h"
#include "arena.h"
#include "diagnostic.h"
#include "lexer.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static struct token peek_at(struct parser_context *p, unsigned n)
{
    assert(n < 4);

    while (p->token_queue_nr <= n)
        p->token_queue[p->token_queue_nr++] = lexer_get_next_token(p->lexer);

    return p->token_queue[n];
}

static struct token pop_token(struct parser_context *p)
{
    assert(p->token_queue_nr);

    struct token poped = *p->token_queue;

    for (size_t i = 1; i < p->token_queue_nr; i++) {
        p->token_queue[i - 1] = p->token_queue[i];
    }

    p->token_queue_nr--;
    return poped;
}

/*
 * Advances the current token with the next one.
 * Returns the previous "current" token.
 */
static struct token advance_token(struct parser_context *p)
{
    p->prev = p->current;
    p->current = p->token_queue_nr ? pop_token(p) : lexer_get_next_token(p->lexer);

    if (p->prev.type == TK_UNINIT)
        p->prev = p->current;

    return p->prev;
}

static int check_token_type(struct parser_context *p, enum token_type type)
{
    return p->current.type == type;
}

static int check_type_and_advance(struct parser_context *p, enum token_type type)
{
    if (!check_token_type(p, type))
        return 0;
    advance_token(p);
    return 1;
}

static struct source_location loc_from_token(struct parser_context *p)
{
    struct token tok = p->prev;
    return (struct source_location){
        .file = p->lexer->file,
        .line_start = tok.lex - tok.col,
        .line = tok.line,
        .col = tok.col,
        .len = (int)tok.len,
    };
}

static void expect_token(struct parser_context *p, enum token_type type)
{
    if (!check_type_and_advance(p, type))
        diag_emit(p->diag, ERROR, loc_from_token(p), "expected '%s' after '%.*s'",
                  lexer_get_token_pretty(type), (int)p->prev.len, p->prev.lex);
}

static struct node *create_node(struct parser_context *p, enum node_type type, struct token token)
{
    struct node *node = arena_alloc(p->arena, sizeof(struct node));
    memset(node, 0, sizeof(struct node));
    node->tok = token;
    node->type = type;

    return node;
}

static void append_node(struct parser_context *p, struct node_list *list, struct node *to_append)
{
    if (list->nr >= list->alloc)
        ARENA_ALLOC_GROW(p->arena, list->items, list->nr + 1, list->alloc);

    list->items[list->nr++] = to_append;
}

static struct node *parse_stmt(struct parser_context *p);
static struct node *parse_expr(struct parser_context *p);

/*
 * block = LBRACE stmt* RBRACE
 */
static struct node *parse_block(struct parser_context *p)
{
    struct node *block = create_node(p, NODE_BLOCK, p->current);
    expect_token(p, TK_LBRACE);

    while (p->current.type != TK_RBRACE)
        append_node(p, &block->block.stmts, parse_stmt(p));
    expect_token(p, TK_RBRACE);
    return block;
}

/*
 * "ret" [ expr ]
 */
static struct node *parse_return(struct parser_context *p)
{
    struct node *ret_node = create_node(p, NODE_RETURN, p->current);
    expect_token(p, TK_RETURN);

    if (!check_token_type(p, TK_SEMICOLON))
        ret_node->return_stmt.expr = parse_expr(p);

    return ret_node;
}

/*
 * x :: expr          compile-time constant (aliases, functions)
 * x := expr          runtime binding, range inferred
 * x : T = expr       runtime binding, range declared
 * x : T : expr       compile-time constant, range declared
 */
static struct node *parse_binding(struct parser_context *p)
{
    struct node *binding = create_node(p, NODE_BINDING, p->current);

    /* Whether it is ::, := or : is already checked at parse_stmt() */
    advance_token(p);

    switch (advance_token(p).type) {
    case TK_WALRUS: /* x :: e */
        break;
    case TK_COLON: /* x := e */
        binding->binding.ann = parse_expr(p);
        if (check_type_and_advance(p, TK_COLON))
            binding->binding.is_const = true;
        else
            expect_token(p, TK_EQUAL);
        break;
    case TK_DECL: /* x : T = e || x : T : e */
        binding->binding.is_const = true;
        break;
    default:
        BUG("parse_binding: unexpected '%s'", lexer_get_token_pretty(p->current.type));
    }

    binding->binding.init = parse_expr(p);
    return binding;
}

static struct node *parse_expr_stmt(struct parser_context *p)
{
    struct node *node = create_node(p, NODE_ERROR, p->current);
    advance_token(p);
    return node;
}

static struct node *parse_expr(struct parser_context *p)
{
    struct node *node = create_node(p, NODE_ERROR, p->current);
    advance_token(p);
    return node;
}

static int is_binding_op(enum token_type type)
{
    return type == TK_DECL || type == TK_WALRUS || type == TK_COLON;
}

/*
 * All stmts ends in ";", NEEDSWORK: some exceptions can obviously be done such as excluding
 * blocks.
 *
 * stmt = ( binding
 *      | "ret" [ expr ]
 *      | if_stmt
 *      | block
 *      | expr ) ";" ;
 */
static struct node *parse_stmt(struct parser_context *p)
{
    struct node *stmt;

    switch (p->current.type) {
    case TK_RETURN:
        stmt = parse_return(p);
        break;
    case TK_LBRACE:
        stmt = parse_block(p);
        break;
    default:
        if (p->current.type == TK_ID && is_binding_op(peek_at(p, 0).type))
            stmt = parse_binding(p);
        else
            stmt = parse_expr_stmt(p);
    }

    expect_token(p, TK_SEMICOLON);
    return stmt;
}

/*
 * NODE_PROGRAM is just a cool block
 * program = stmt*
 */
static struct node *parse_program(struct parser_context *p)
{
    struct token program_tok = TOKEN_INIT;
    struct node *program;
    advance_token(p);

    program = create_node(p, NODE_PROGRAM, program_tok);
    while (!check_token_type(p, TK_EOF)) {
        struct node *stmt = parse_stmt(p);

        if (stmt)
            append_node(p, &program->block.stmts, stmt);
    }

    return program;
}

struct node *parser_parse(struct parser_context *p)
{
    return parse_program(p);
}

static void print_indent(size_t depth)
{
    for (size_t i = 0; i < depth; i++)
        printf("    ");
}

static void print_node(struct node *n, size_t depth)
{
    if (!n)
        return;

    print_indent(depth);

    switch (n->type) {
    case NODE_PROGRAM:
    case NODE_BLOCK:
        printf("%s\n", n->type == NODE_PROGRAM ? "[PROGRAM]" : "[BLOCK]");
        foreach_node(node, n->block.stmts) print_node(node, depth + 1);
        break;
    case NODE_RETURN:
        printf("[RETURN]\n");
        print_node(n->return_stmt.expr, depth + 1);
        break;
    case NODE_ERROR:
        printf("[ERROR]  lexeme='%.*s'\n", (int)n->tok.len, n->tok.lex);
        break;
    default:
        printf("[?? type=%d]\n", (int)n->type);
    }
}

void parser_print(struct node *program)
{
    assert(program);
    print_node(program, 0);
}

void parser_init(struct parser_context *p, struct lexer_context *lexer, struct arena *arena,
                 struct diag_context *diag)
{
    struct token t = TOKEN_INIT;

    p->lexer = lexer;
    p->arena = arena;
    p->diag = diag;
    p->current = t;
}
