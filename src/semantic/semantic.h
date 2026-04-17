#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "semantic/type.h"
#include "lexer/lexer.h"

struct ast_node;
struct compiler_context;
struct scope;

enum symbol_type {
	SYM_VAR,
	SYM_CONST,
	SYM_PARAM,
	SYM_FN,
	SYM_STRUCT,
	SYM_ENUM,
	SYM_TYPE_ALIAS,
	SYM_STRUCT_FIELD,
	SYM_ENUM_MEMBER,
};

struct symbol {
	enum symbol_type kind;
	struct token tok;
	struct type *type;
	struct ast_node *node;
	/*
	 * Aliases can call between them in cycles:
	 *
	 * type a = b
	 *
	 * type b = a
	 *
	 * This needs to be detected to avoid infinite recursion.
	 *
	 * 0 = not resolved,
	 * 1 = resolving,
	 * 2 = resolved.
	 */
	unsigned resolved_state:2;

	union {
		struct {
			unsigned is_mut:1;
			unsigned is_init:1;
		} var;
		struct {
			struct symbol **params;
			size_t nr_param;
			struct symbol **locals;
			size_t nr_locals;
			size_t alloc_locals;
		} fn;
		struct {
			struct symbol **members;
			size_t nr;
		} aggregate;
		struct {
			long long val;
			struct symbol *parent;
			unsigned explicit:1;
		} enum_member;
		struct {
			size_t index;
			struct symbol *parent;
		} field;
	};
};

struct scope {
	struct scope *parent;
	struct symbol **symbols;
	size_t nr;
	size_t alloc;
};

struct semantic_context {
	struct compiler_context *cc;

	struct scope *global;
	struct scope *current;

	struct type *t_err;
	struct type *t_int;
	struct type *t_uint;
	struct type *t_float;
	struct type *t_double;
	struct type *t_bool;
	struct type *t_char;
	struct type *t_string;
	struct type *t_void;
	struct type *t_null;

	struct type **ptrs;
	size_t nr_ptr;
	size_t alloc_ptr;

	struct type **arrs;
	size_t nr_arr;
	size_t alloc_arr;

	struct type **tuples;
	size_t nr_tuple;
	size_t alloc_tuple;

	struct type **fns;
	size_t nr_fn;
	size_t alloc_fn;

	struct symbol *current_fn;
	int loop_depth;
	unsigned in_defer:1;
};

void sem_init(struct semantic_context *sc, struct compiler_context *cc);
void semantic_analyze(struct semantic_context *sc, struct ast_node *program);

#endif
