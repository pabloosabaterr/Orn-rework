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
    NODE_LOOP,
    NODE_RETURN,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_LABEL,
    NODE_GOTO,
    NODE_EXPR_STMT,

    NODE_IF,
    NODE_BINARY,
    NODE_UNARY,
    NODE_ASSIGN,
    NODE_CALL,
    NODE_ARG,
    NODE_INDEX,
    NODE_MEMBER,
    NODE_CAST_OR_CALL,
    NODE_CAST,

    NODE_FN,
    NODE_PARAM,
    NODE_OBJ,
    NODE_ENUM,
    NODE_VARIANT,
    NODE_IMPORT,

    NODE_INT,
    NODE_FLOATING,
    NODE_STRING,
    NODE_CHAR,
    NODE_BOOL,
    NODE_NULL,
    NODE_ID,
    NODE_PRIMITIVE,
    NODE_HOLE,
    NODE_DOT_ID,
    NODE_TUPLE,
    NODE_ARRAY,
    NODE_TYPE_ARR,
};

enum op_type {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_EQ,
    OP_NEQ,
    OP_LT,
    OP_GT,
    OP_LE,
    OP_GE,
    OP_AND,
    OP_OR,
    OP_BIT_AND,
    OP_BIT_OR,
    OP_BIT_XOR,
    OP_LSHIFT,
    OP_RSHIFT,
    OP_RANGE,
    OP_NEG,
    OP_NOT,
    OP_BIT_NOT,
    OP_ADDR,
    OP_DEREF,
    OP_ASSIGN,
    OP_PLUSEQ,
    OP_MINUSEQ,
    OP_STAREQ,
    OP_SLASHEQ,
    OP_MODEQ,
};

/*
 * It is preferred to be redundant at the union so later on the next stages,
 * the code is readable and there is no need for node_type dispatchers.
 */
struct ast_node {
    enum node_type type;
    struct token tok;
    struct type *rtype;
    struct symbol *rsym;
    union {
        /* NODE_PROGRAM, NODE_BLOCK */
        struct {
            struct ast_node **childs;
            size_t nr;
            size_t alloc;
        } block;

        /*
         * The single rule. Name is in tok.
         *
         *   ann  init  is_const
         *   NULL  set     1      X :: expr
         *   NULL  set     0      X := expr
         *   set  NULL     -      X : T
         *   set   set     1      X : T : expr
         *   set   set     0      X : T = expr
         *
         * is_method is set for a member written with a leading dot.
         */
        struct {
            struct ast_node *ann;
            struct ast_node *init;
            unsigned is_const : 1;
            unsigned is_method : 1;
        } binding;

        /* NODE_FN. The name, if any, belongs to the binding above. */
        struct {
            struct ast_node **params;
            size_t nr_param;
            struct ast_node *ret_type;
            struct ast_node *body;
            unsigned is_variadic : 1;
        } fn;

        /*
         * NODE_OBJ, NODE_ENUM, NODE_TUPLE, NODE_ARRAY
         *
         * An enum holds its variants first, then its members.
         */
        struct {
            struct ast_node **items;
            size_t nr_item;
        } list;

        /*
         * loop            var NULL, head NULL
         * loop cond       var NULL, head set
         * loop i := range var set,  head set
         */
        struct {
            struct token *var;
            struct ast_node *head;
            struct ast_node *body;
        } loop;

        struct {
            struct ast_node *cond;
            struct ast_node *then_body;
            struct ast_node *else_body;
        } if_stmt;

        /* NODE_CAST_OR_CALL until the semantic pass collapses it */
        struct {
            struct ast_node *expr;
            struct ast_node *target_type;
        } cast;

        struct {
            struct ast_node *expr;
        } return_stmt;
        struct {
            struct ast_node *expr;
        } expr_stmt;
        struct {
            struct ast_node *expr;
        } import_expr;
        struct {
            struct ast_node *val;
        } variant;
        struct {
            struct ast_node *left;
        } member;
        struct {
            struct ast_node *arg;
        } named_arg;

        struct {
            enum op_type type;
            struct ast_node *operand;
        } unary;

        struct {
            struct ast_node *left;
            struct ast_node *right;
            enum op_type type;
        } binary;

        struct {
            enum op_type type;
            struct ast_node *target;
            struct ast_node *val;
        } assign;

        struct {
            struct ast_node *callee;
            struct ast_node **args;
            size_t nr_arg;
        } call;

        /* end is NULL for plain index */
        struct {
            struct ast_node *obj;
            struct ast_node *idx;
            struct ast_node *end;
        } index;

        struct {
            long long val;
        } lit_int;

        struct {
            double val;
        } lit_floating;

        /* NODE_STRING, NODE_CHAR */
        struct {
            const char *val;
            size_t len;
        } lit_str;

        struct {
            unsigned val : 1;
        } lit_bool;

        /* NODE_PARAM */
        struct {
            struct ast_node *ann;
            unsigned is_spread : 1;
        } param;

        /* [size]elem_type, size is an expression */
        struct {
            struct ast_node *elem_type;
            struct ast_node *size;
        } type_array;
    };
};

struct parser_context {
    struct lexer_context *lexer;
    struct diag_context *diag;
    const char *file;
    struct token current;
    struct token prev;
    struct arena *arena;
    int errors;
    unsigned no_struct_init : 1;
    unsigned in_panic : 1;
};

void parser_init(struct parser_context *ctx, struct lexer_context *lexer,
                 struct compiler_context *cc);
struct ast_node *parser_parse(struct parser_context *parser);
void parser_free(struct parser_context *parser);

#endif
