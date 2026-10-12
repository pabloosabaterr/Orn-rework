#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "frontend/parser.h"
#include "support/arena.h"
#include "frontend/diagnostic.h"
#include "frontend/lexer.h"

static struct token peek(struct parser_context *p, unsigned n)
{
    assert(n < 4);

    while (p->token_queue_nr <= n)
        p->token_queue[p->token_queue_nr++] = next_token(p->lexer);

    return p->token_queue[n];
}

/*
 * Advances the current token with the next one.
 * Returns the previous "current" token.
 */
static struct token advance(struct parser_context *p)
{
    p->prev = p->current;
    if (p->token_queue_nr) {
        struct token poped = *p->token_queue;

        for (size_t i = 1; i < p->token_queue_nr; i++) {
            p->token_queue[i - 1] = p->token_queue[i];
        }

        p->token_queue_nr--;
        p->current = poped;
    } else {
        next_token(p->lexer);
    }

    if (p->prev.type == TK_UNINIT)
        p->prev = p->current;

    return p->prev;
}

/* Is the current token of this type? */
static bool check(struct parser_context *p, enum token_type type)
{
    return p->current.type == type;
}

/* Like check(), but also consumes the token when it matches. */
static bool match(struct parser_context *p, enum token_type type)
{
    if (!check(p, type))
        return false;
    advance(p);
    return true;
}

static struct source_location loc_of(struct parser_context *p, struct token tok)
{
    return (struct source_location){
        .file = p->lexer->file,
        .line_start = tok.lex - tok.col,
        .line = tok.line,
        .col = tok.col,
        .len = (int)tok.len,
    };
}

static struct source_location loc_from_token(struct parser_context *p)
{
    return loc_of(p, p->prev);
}

static void expect_token(struct parser_context *p, enum token_type type)
{
    if (!match(p, type))
        diag_emit(p->diag, ERROR, loc_from_token(p), "expected '%s' after '%.*s'",
                  token_pretty(type), (int)p->prev.len, p->prev.lex);
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

    while (!check(p, TK_RBRACE) && !check(p, TK_EOF))
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

    if (!check(p, TK_SEMICOLON))
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
    struct token op;

    /* Whether it is ::, := or : is already checked at parse_stmt() */
    advance(p); /* skip the name */
    op = advance(p);

    switch (op.type) {
    case TK_DECL: /* x :: e */
        binding->binding.is_const = true;
        break;
    case TK_WALRUS: /* x := e */
        break;
    case TK_COLON: /* x : T = e  or  x : T : e */
        binding->binding.ann = parse_expr(p);
        if (match(p, TK_COLON))
            binding->binding.is_const = true;
        else
            expect_token(p, TK_EQUAL);
        break;
    default:
        BUG("parse_binding: unexpected '%s'", token_pretty(op.type));
    }

    binding->binding.init = parse_expr(p);
    return binding;
}

static struct node *parse_expr_stmt(struct parser_context *p)
{
    struct node *node = create_node(p, NODE_EXPR_STMT, p->current);
    node->expr_stmt.expr = parse_expr(p);
    return node;
}

static struct node *parse_expr_internal(struct parser_context *p, int min_binding);

static const struct binding_powers binding_table[] = {
    [OP_OR] = { 1, 2 },    [OP_AND] = { 3, 4 },   [OP_EQ] = { 5, 6 },   [OP_NEQ] = { 5, 6 },
    [OP_LT] = { 5, 6 },    [OP_GT] = { 5, 6 },    [OP_LE] = { 5, 6 },   [OP_GE] = { 5, 6 },
    [OP_RANGE] = { 7, 8 }, [OP_ADD] = { 9, 10 },  [OP_SUB] = { 9, 10 }, [OP_MUL] = { 11, 12 },
    [OP_DIV] = { 11, 12 }, [OP_MOD] = { 11, 12 },
};

static const int prefix_table[] = {
    [OP_NEG] = 13,
    [OP_NOT] = 13,
};

#define CALL_BINDING 15

static bool is_non_assoc(enum op_type op)
{
    switch (op) {
    case OP_EQ:
    case OP_NEQ:
    case OP_LT:
    case OP_GT:
    case OP_LE:
    case OP_GE:
    case OP_RANGE:
        return true;
    default:
        return false;
    }
}

static struct node *parse_param(struct parser_context *p)
{
    struct node *node = create_node(p, NODE_PARAM, p->current);
    advance(p);
    expect_token(p, TK_COLON);
    node->param.ann = parse_expr(p);

    return node;
}

static struct node *parse_fn(struct parser_context *p)
{
    struct node *node = create_node(p, NODE_FN, p->prev);
    if (!check(p, TK_RPAREN)) {
        do
            append_node(p, &node->fn.params, parse_param(p));
        while (match(p, TK_COMMA));
    }
    expect_token(p, TK_RPAREN);

    node->fn.ret_type = parse_expr(p);
    node->fn.body = parse_block(p);

    return node;
}

static struct node *parse_prefix(struct parser_context *p)
{
    struct node *node = NULL;

    switch (p->current.type) {
    case TK_ID:
        node = create_node(p, NODE_ID, p->current);
        advance(p);
        break;
    case TK_NUMBER:
    case TK_HEX:
    case TK_OCTAL:
    case TK_BINARY:
        node = create_node(p, NODE_NUMBER, p->current);
        advance(p);
        break;
    case TK_TRUE:
    case TK_FALSE:
        node = create_node(p, NODE_BOOL, p->current);
        node->lit_bool.val = check(p, TK_TRUE);
        advance(p);
        break;
    case TK_LPAREN:
        advance(p);
        if (check(p, TK_RPAREN) || (check(p, TK_ID) && peek(p, 0).type == TK_COLON)) {
            node = parse_fn(p);
        } else {
            node = parse_expr(p);
            expect_token(p, TK_RPAREN);
        }
        break;
    case TK_MINUS:
    case TK_NOT:
        node = create_node(p, NODE_UNARY, p->current);
        node->unary.type = check(p, TK_MINUS) ? OP_NEG : OP_NOT;
        advance(p);
        node->unary.operand = parse_expr_internal(p, prefix_table[node->unary.type]);
        break;
    default:
        node = create_node(p, NODE_ERROR, p->current);
        diag_emit(p->diag, ERROR, loc_of(p, p->current), "Unrecognized expr prefix");
        if (!check(p, TK_SEMICOLON) && !check(p, TK_EOF))
            advance(p);
        break;
    }

    return node;
}

static struct node *build_op_node(struct parser_context *p, struct token tok, enum op_type op,
                                  struct node *lhs, struct node *rhs)
{
    struct node *node = create_node(p, NODE_BINARY, tok);
    node->binary.type = op;
    node->binary.left = lhs;
    node->binary.right = rhs;
    return node;
}

static int token_to_binop(enum token_type type, enum op_type *op)
{
    switch (type) {
    case TK_PLUS:
        *op = OP_ADD;
        return true;
    case TK_MINUS:
        *op = OP_SUB;
        return true;
    case TK_STAR:
        *op = OP_MUL;
        return true;
    case TK_SLASH:
        *op = OP_DIV;
        return true;
    case TK_MOD:
        *op = OP_MOD;
        return true;
    case TK_CMP:
        *op = OP_EQ;
        return true;
    case TK_NEQ:
        *op = OP_NEQ;
        return true;
    case TK_LT:
        *op = OP_LT;
        return true;
    case TK_GT:
        *op = OP_GT;
        return true;
    case TK_LE:
        *op = OP_LE;
        return true;
    case TK_GE:
        *op = OP_GE;
        return true;
    case TK_AND:
        *op = OP_AND;
        return true;
    case TK_OR:
        *op = OP_OR;
        return true;
    case TK_RANGE:
        *op = OP_RANGE;
        return true;
    default:
        return false;
    }
}

/*
 * call = expr "(" [ expr { "," expr } ] ")"
 */
static struct node *parse_call(struct parser_context *p, struct node *callee)
{
    struct node *call = create_node(p, NODE_CALL, p->current);
    call->call.callee = callee;
    expect_token(p, TK_LPAREN);

    if (!check(p, TK_RPAREN)) {
        do
            append_node(p, &call->call.args, parse_expr(p));
        while (match(p, TK_COMMA));
    }
    expect_token(p, TK_RPAREN);

    call->call.nr_arg = call->call.args.nr;
    return call;
}

static struct node *parse_expr_internal(struct parser_context *p, int min_binding)
{
    struct node *rhs, *lhs = parse_prefix(p);

    for (;;) {
        struct binding_powers bp;
        struct token op_tok = p->current;
        enum op_type op, next;

        if (check(p, TK_LPAREN)) {
            if (CALL_BINDING < min_binding)
                break;
            lhs = parse_call(p, lhs);
            continue;
        }

        if (!token_to_binop(p->current.type, &op))
            break;

        bp = binding_table[op];
        if (bp.left < min_binding)
            break;

        advance(p);
        rhs = parse_expr_internal(p, bp.right);

        lhs = build_op_node(p, op_tok, op, lhs, rhs);

        /*
         * a < b < c and a..b..c are errors. The loop would group them to the
         * left, so report it and keep going as if it were left associative.
         */
        if (is_non_assoc(op) && token_to_binop(p->current.type, &next) &&
            binding_table[next].left == bp.left)
            diag_emit(p->diag, ERROR, loc_of(p, p->current), "'%.*s' cannot be chained",
                      (int)p->current.len, p->current.lex);
    }

    return lhs;
}

static struct node *parse_expr(struct parser_context *p)
{
    return parse_expr_internal(p, 0);
}

static struct node *parse_if(struct parser_context *p)
{
    struct node *node = create_node(p, NODE_ERROR, p->current);
    advance(p);
    return node;
}

static struct node *parse_loop(struct parser_context *p)
{
    struct node *node = create_node(p, NODE_ERROR, p->current);
    advance(p);
    return node;
}

static int is_binding_op(enum token_type type)
{
    return type == TK_DECL || type == TK_WALRUS || type == TK_COLON;
}

typedef struct node *parsing_fn(struct parser_context *);

static parsing_fn *const stmt_vtable[] = {
    [TK_RETURN] = parse_return,
    [TK_LBRACE] = parse_block,
    [TK_IF] = parse_if,
    [TK_LOOP] = parse_loop,
};

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
    enum token_type type = p->current.type;
    struct node *stmt;
    parsing_fn *fn;

    fn = type < ARRAY_SIZE(stmt_vtable) ? stmt_vtable[type] : NULL;

    if (fn)
        stmt = fn(p);
    else if (check(p, TK_ID) && is_binding_op(peek(p, 0).type))
        stmt = parse_binding(p);
    else
        stmt = parse_expr_stmt(p);

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
    advance(p);

    program = create_node(p, NODE_PROGRAM, program_tok);
    while (!check(p, TK_EOF)) {
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
    case NODE_FN:
        printf("[FN]\n");
        foreach_node(node, n->fn.params) print_node(node, depth + 1);
        if (n->fn.ret_type) {
            print_indent(depth + 1);
            printf("[RET]\n");
            print_node(n->fn.ret_type, depth + 2);
        }
        print_node(n->fn.body, depth + 1);
        break;
    case NODE_PARAM:
        printf("[PARAM] '%.*s'\n", (int)n->tok.len, n->tok.lex);
        print_node(n->param.ann, depth + 1);
        break;
    case NODE_RETURN:
        printf("[RETURN]\n");
        print_node(n->return_stmt.expr, depth + 1);
        break;
    case NODE_EXPR_STMT:
        printf("[EXPR_STMT]\n");
        print_node(n->expr_stmt.expr, depth + 1);
        break;
    case NODE_BINDING:
        printf("[BINDING] '%.*s'%s\n", (int)n->tok.len, n->tok.lex,
               n->binding.is_const ? " const" : "");
        if (n->binding.ann) {
            print_indent(depth + 1);
            printf("[ANN]\n");
            print_node(n->binding.ann, depth + 2);
        }
        print_node(n->binding.init, depth + 1);
        break;
    case NODE_BINARY:
        printf("[BINARY] '%.*s'\n", (int)n->tok.len, n->tok.lex);
        print_node(n->binary.left, depth + 1);
        print_node(n->binary.right, depth + 1);
        break;
    case NODE_UNARY:
        printf("[UNARY] '%.*s'\n", (int)n->tok.len, n->tok.lex);
        print_node(n->unary.operand, depth + 1);
        break;
    case NODE_CALL:
        printf("[CALL]\n");
        print_node(n->call.callee, depth + 1);
        foreach_node(node, n->call.args) print_node(node, depth + 1);
        break;
    case NODE_ID:
        printf("[ID] '%.*s'\n", (int)n->tok.len, n->tok.lex);
        break;
    case NODE_NUMBER:
        printf("[NUMBER] '%.*s'\n", (int)n->tok.len, n->tok.lex);
        break;
    case NODE_BOOL:
        printf("[BOOL] '%.*s'\n", (int)n->tok.len, n->tok.lex);
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
