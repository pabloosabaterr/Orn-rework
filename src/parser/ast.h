#ifndef AST_H
#define AST_H

#include "lexer/lexer.h"
#include "memory/arena.h"

#include <stddef.h>

enum node_type {
	NODE_PROGRAM,

	NODE_FN_DEC,
	NODE_STRUCT_DEC,
	NODE_IMPL_DEC,
	NODE_ENUM_DEC,
	NODE_TYPE_DEC,
	NODE_LET_DEC,
	NODE_CONST_DEC,
	NODE_IMPORT_DEC,

	NODE_BLOCK,
	NODE_IF,
	NODE_WHILE,
	NODE_FOR,
	NODE_RETURN,
	NODE_BREAK,
	NODE_CONTINUE,
	NODE_DEFER,
	NODE_MATCH,
	NODE_INCDEC,
	NODE_EXPR_STMT,

	NODE_BINARY,
	NODE_UNARY,
	NODE_CALL,
	NODE_CAST,
	NODE_INDEX,
	NODE_MEMBER,
	NODE_NAMESPACE,
	NODE_ASSIGN,
	NODE_INT,
	NODE_FLOATING,
	NODE_STRING,
	NODE_CHAR,
	NODE_BOOL,
	NODE_NULL,
	NODE_ID,
	NODE_SIZEOF,
	NODE_SYSCALL,
	NODE_STRUCT_INIT,
	NODE_ARRAY_INIT,

	NODE_TYPE_NAME,
	NODE_TYPE_PTR,
	NODE_TYPE_ARR,
	NODE_TYPE_TUPLE,

	NODE_PARAM,
	NODE_FIELD,
	NODE_ENUM_MEMBER,
	NODE_MATCH_ARM,
	NODE_MATCH_PATTERN,
	NODE_FIELD_INIT,
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

	OP_INC,
	OP_DEC,
};

/*
 * It is preferred to be redundant at the union so later on the next stages,
 * the code is readable and there is no need for node_type dispatchers.
 *
 * A single node is 80 bytes on 64-bit.
 */
struct ast_node {
	enum node_type type;
	struct token tok;
	union {
		/* NODE_PROGRAM, NODE_BLOCK */
		struct {
			struct ast_node **childs;
			size_t nr;
			size_t alloc;
		} block;

		struct {
			struct ast_node **params;
			size_t nr_param;
			struct ast_node *ret_type;
			struct ast_node *body;
			unsigned is_variadic:1;
		} fn_dec;

		/*
		 * NODE_STRUCT_DEC, NODE_IMPL_DEC, NODE_ENUM_DEC,
		 * NODE_ARRAY_INIT, NODE_TYPE_TUPLE, NODE_SYSCALL
		 * NODE_STRUCT_INIT
		 */
		struct {
			struct ast_node **items;
			size_t nr_item;
		} list;

		struct {
			struct ast_node **assocs;
			size_t nr_assoc;
			struct ast_node *val;
		} enum_member;

		struct {
			struct token *name;
			size_t nr_name;
			struct ast_node *ann;
			struct ast_node *init;
		} let_dec;

		struct {
			struct ast_node *cond;
			struct ast_node *body;
		} while_stmt;

		struct {
			struct ast_node *pattern;
			struct ast_node *body;
		} match_arm;

		struct {
			struct ast_node *ann;
			struct ast_node *init;
		} const_dec;

		struct {
			struct ast_node *expr;
			struct ast_node *target_type;
		} cast;

		struct {
			struct ast_node **conds;
			struct ast_node **bodies;
			size_t nr_branch;
			struct ast_node *else_body;
		} if_stmt;

		struct {
			struct ast_node *range_lo;
			struct ast_node *range_hi;
			struct ast_node *body;
		} for_stmt;

		struct { struct ast_node *expr; } return_stmt;
		struct { struct ast_node *stmt; } defer_stmt;
		struct { struct ast_node *expr; } expr_stmt;
		struct { struct ast_node *type; } type_ptr;
		struct { struct ast_node *type; } type_dec;
		struct { struct ast_node *expr; } sizeof_expr;
		struct { struct ast_node *val;  } field_init;
		struct { struct ast_node *left; } member;
		struct { struct ast_node *left; } namespace;

		struct {
			struct ast_node *subj;
			struct ast_node **arms;
			size_t nr_arm;
		} match;

		struct {
			struct token *bind;
			size_t nr_bind;
			struct ast_node *expr;
			unsigned is_wildcard:1;
		} match_pattern;

		struct {
			enum op_type type;
			struct ast_node *operand;
		} unary;

		struct {
			enum op_type type;
			struct ast_node *target;
		} incdec;

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
			const char *val;
		} lit_char;

		struct {
			unsigned val:1;
		} lit_bool;

		/* NODE_PARAM, NODE_FIELD */
		struct {
			struct ast_node *ann;
		} typed;

		struct {
			struct ast_node *elem_type;
			long long size;
		} type_array;
	};
};

struct ast_node *ast_node_new(struct arena *a, enum node_type type,
			      struct token tok);
void ast_node_append(struct arena *a, struct ast_node *block,
		     struct ast_node *child);
enum op_type token_to_op(enum token_type type);
void ast_dump(struct ast_node *node);

#endif
