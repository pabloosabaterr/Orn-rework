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

static struct token next_token(struct parser_context *p)
{
    return p->token_queue_nr ? pop_token(p) : lexer_get_next_token(p->lexer);
}

static struct token advance_token(struct parser_context *p)
{
    p->prev = p->current;
    p->current = next_token(p);

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

static struct parser_ast_node *create_node(struct parser_context *p, enum node_type type,
                                           struct token token)
{
    struct parser_ast_node *node = arena_alloc(p->arena, sizeof(struct parser_ast_node));
    memset(node, 0, sizeof(struct parser_ast_node));
    node->tok = token;
    node->type = type;

    return node;
}

static void append_node(struct parser_context *p,
                       struct node_list *list,
                       struct parser_ast_node *to_append)
{
    if (list->nr >= list->alloc)
        ARENA_ALLOC_GROW(p->arena, list->items, list->nr + 1, list->alloc);

    list->items[list->nr++] = to_append;
}

static struct parser_ast_node *parse_stmt(struct parser_context *p);

/*
 * block = LBRACE stmt* RBRACE
 */
static struct parser_ast_node *parse_block(struct parser_context *p)
{
    struct parser_ast_node *block = create_node(p, NODE_BLOCK, p->current);
    expect_token(p, TK_LBRACE);

    while (p->current.type != TK_RBRACE)
        append_node(p, &block->block.stmts, parse_stmt(p));
    expect_token(p, TK_RBRACE);
    return block;
}

static struct parser_ast_node *parse_return(struct parser_context *p)
{
    struct parser_ast_node *node = create_node(p, NODE_ERROR, p->current);
    advance_token(p);
    return node;
}
static struct parser_ast_node *parse_binding(struct parser_context *p)
{
    struct parser_ast_node *node = create_node(p, NODE_ERROR, p->current);
    advance_token(p);
    return node;
}
static struct parser_ast_node *parse_expr_stmt(struct parser_context *p)
{
    struct parser_ast_node *node = create_node(p, NODE_ERROR, p->current);
    advance_token(p);
    return node;
}

/*
 * stmt = binding SEMI
 *      | RET expr? SEMI
 *      | expr SEMI
 *      | block
 */
static struct parser_ast_node *parse_stmt(struct parser_context *p)
{
    switch (p->current.type) {
    case TK_RETURN:
        return parse_return(p);
    case TK_LBRACE:
        return parse_block(p);
    default:
        if (p->current.type == TK_ID) {
            switch (peek_at(p, 0).type) {
            case TK_DECL:
            case TK_WALRUS:
            case TK_COLON:
                return parse_binding(p);
            default:
                break;
            }
        }
        return parse_expr_stmt(p);
    }
}

/*
 * NODE_PROGRAM is just a cool block
 * program = stmt*
 */
static struct parser_ast_node *parse_program(struct parser_context *p)
{
    struct token program_tok = TOKEN_INIT;
    struct parser_ast_node *program;
    advance_token(p);

    program = create_node(p, NODE_PROGRAM, program_tok);
    while (!check_token_type(p, TK_EOF)) {
        struct parser_ast_node *stmt = parse_stmt(p);

        if (stmt)
            append_node(p, &program->block.stmts, stmt);
    }

    return program;
}

struct parser_ast_node *parser_parse(struct parser_context *p)
{
    return parse_program(p);
}

static void print_indent(size_t depth)
{
    for (size_t i = 0; i < depth; i++)
        printf("    ");
}

static void print_node(struct parser_ast_node *n, size_t depth)
{
    if (!n)
        return;

    print_indent(depth);

    switch (n->type) {
    case NODE_PROGRAM:
    case NODE_BLOCK:
        printf("%s\n", n->type == NODE_PROGRAM ? "[PROGRAM]" : "[BLOCK]");
        foreach_node(node, n->block.stmts)
            print_node(node, depth + 1);
        break;

    case NODE_ERROR:
        printf("[ERROR]  lexeme='%.*s'\n", (int)n->tok.len, n->tok.lex);
        break;

    default:
        printf("[?? type=%d]\n", (int)n->type);
    }
}

void parser_print(struct parser_ast_node *program)
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
