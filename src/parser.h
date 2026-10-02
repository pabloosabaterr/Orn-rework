#ifndef PARSER_H
#define PARSER_H

#include "diagnostic.h"
#include "lexer.h"

struct compiler_context;
struct diag_context;
struct arena;

enum node_type {
    NODE_PROGRAM,
    NODE_ERROR,

    NODE_BINDING,

    NODE_BLOCK,
    NODE_RETURN,
    NODE_EXPR_STMT,

    NODE_IF,
    NODE_BINARY,
    NODE_UNARY,
    NODE_CALL,

    NODE_FN,
    NODE_PARAM,

    NODE_INT,
    NODE_BOOL,
    NODE_ID,
};

enum op_type {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_EQ,
    OP_NEQ,
    OP_LT,
    OP_GT,
    OP_LE,
    OP_GE,
    OP_AND,
    OP_OR,
    OP_RANGE,
    OP_NEG,
    OP_NOT,
};

struct node_list {
    struct node **items;
    size_t nr;
    size_t alloc;
};

/*
 * It is preferred to be redundant at the union so later on the next stages,
 * the code is readable and there is no need for node_type dispatchers.
 */
struct node {
    enum node_type type;
    struct token tok;
    union {
        /* NODE_PROGRAM, NODE_BLOCK */
        struct {
            struct node_list stmts;
        } block;

        /*
         * The single rule. Name is in tok.
         *
         *   ann  init  is_const
         *   NULL  set     1      X :: expr
         *   NULL  set     0      X := expr
         *   set   set     1      X : T : expr
         *   set   set     0      X : T = expr
         *
         * ann is an expression, so a range works as a type: X : 0..255 = 3
         * and an alias is just a constant binding: u8 :: 0..255
         */
        struct {
            struct node *ann;
            struct node *init;
            unsigned is_const : 1;
        } binding;

        /* NODE_FN. The name, if any, belongs to the binding above. */
        struct {
            struct node_list params;
            struct node *ret_type;
            struct node *body;
        } fn;

        struct {
            struct node *cond;
            struct node *then_body;
            struct node *else_body;
        } if_stmt;

        struct {
            struct node *expr;
        } return_stmt;
        struct {
            struct node *expr;
        } expr_stmt;

        struct {
            enum op_type type;
            struct node *operand;
        } unary;

        struct {
            struct node *left;
            struct node *right;
            enum op_type type;
        } binary;

        struct {
            struct node *callee;
            struct node_list args;
            size_t nr_arg;
        } call;

        /*
         * NEEDSWORK: range bounds need more than 64 bits once arithmetic
         * is propagated (0..2^63 * 0..2^63). Keep the literal as written
         * here and let the range pass convert it to its big integer type.
         */
        struct {
            long long val;
        } lit_int;

        struct {
            unsigned val : 1;
        } lit_bool;

        /* NODE_PARAM */
        struct {
            struct node *ann;
        } param;
    };
};

#define foreach_node(node, list) \
    for (struct node **_p = (list).items, *node; \
         _p < (list).items + (list).nr && ((node) = *_p, 1); \
         _p++)

#define PARSER_AST_NODE_INIT { .type = NODE_ERROR }

struct parser_context {
    struct lexer_context *lexer;
    struct diag_context *diag;
    struct arena *arena;

    struct token current;
    struct token prev;

    struct token token_queue[4];
    size_t token_queue_nr;
};

#define PARSER_CONTEXT_INIT { 0 }

/*
 * Initialice a given parser context
 */
void parser_init(struct parser_context *ctx, struct lexer_context *lexer, struct arena *arena,
                 struct diag_context *diag);
struct node *parser_parse(struct parser_context *parser);

/*
 * Print to stdout the Abtract Syntax Tree
 */
void parser_print(struct node *program);

#endif
