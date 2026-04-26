#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "memory/hashmap.h"
#include "semantic/type.h"
#include "lexer/lexer.h"
#include <stddef.h>

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
		/*
		 * members keeps declaration order for codegen layout
		 * and iteration. member_hashmap provides O(1) lookup
		 * by name for field resolution and duplicate checks.
		 */
		struct {
			/*
			 * On searchs:
			 * nr works to quickly check if two objs are the
			 * same, only if the nr of members if the same, the
			 * members are compared.
			 *
			 * Same for initializers by comparing if the nr is
			 * the same, then checking if the members are the same.
			 *
			 * A linear array is still needed to keep the order of
			 * the members.
			 */
			struct symbol **members;
			size_t alloc;
			size_t nr;
			struct hashmap member_hashmap;
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
	struct hashmap symbols;
};

struct semantic_context {
	struct compiler_context *cc;

	struct scope *global;
	struct scope *current;

	/*
	 * Expressions like namescpaces T::N, where T can be ommited but
	 * it gets resolved from the declaration, example:
	 *
	 * enum T { N }
	 *
	 * let foo: T = ::N;
	 *
	 * when looking at ::N, it needs the context from the declaration in
	 * order to resolve T.
	 * To avoid having to go though all the scopes and the member of all
	 * the symbols, it get collected here to be used when resolving the
	 * member.
	 *
	 * If there is no context from the declaration, but the function
	 * returns T, it will be resolved with the return type of the function.
	 */
	struct symbol *expected_sym;

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
