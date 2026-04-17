#include "semantic/semantic.h"
#include "diagnostic/diagnostic.h"
#include "memory/arena.h"
#include "compiler.h"
#include "parser/ast.h"
#include "semantic/type.h"
#include "memory/wrapper.h"
#include <string.h>

static struct type *prim_new(struct semantic_context *sc, enum base_type kind)
{
	struct type *t = arena_alloc(&sc->cc->arena, sizeof(*t));
	memset(t, 0, sizeof(*t));
	t->kind = kind;
	return t;
}

static struct scope *scope_new(struct semantic_context *sc, struct scope *parent)
{
	struct scope *s = arena_alloc(&sc->cc->arena, sizeof(*s));
	memset(s, 0, sizeof(*s));
	s->parent = parent;
	return s;
}

void sem_init(struct semantic_context *sc, struct compiler_context *cc)
{
	memset(sc, 0, sizeof(*sc));
	sc->cc = cc;

	sc->t_err = prim_new(sc, TY_ERROR);
	sc->t_int = prim_new(sc, TY_INT);
	sc->t_uint = prim_new(sc, TY_UINT);
	sc->t_float = prim_new(sc, TY_FLOAT);
	sc->t_double = prim_new(sc, TY_DOUBLE);
	sc->t_bool = prim_new(sc, TY_BOOL);
	sc->t_char = prim_new(sc, TY_CHAR);
	sc->t_string = prim_new(sc, TY_STRING);
	sc->t_void = prim_new(sc, TY_VOID);
	sc->t_null = prim_new(sc, TY_NULL);

	sc->global = scope_new(sc, NULL);
	sc->current = sc->global;
}

struct type *type_ptr(struct semantic_context *sc, struct type *pointee)
{
	size_t i;
	struct type *t;

	for (i = 0; i < sc->nr_ptr; i++)
		if (sc->ptrs[i]->ptr.pointee == pointee)
			return sc->ptrs[i];

	t = arena_alloc(&sc->cc->arena, sizeof(*t));
	memset(t, 0, sizeof(*t));

	t->kind = TY_PTR;
	t->ptr.pointee = pointee;

	ARENA_ALLOC_GROW(&sc->cc->arena, sc->ptrs, sc->nr_ptr + 1, sc->alloc_ptr);
	sc->ptrs[sc->nr_ptr++] = t;
	return t;
}

struct type *type_arr(struct semantic_context *sc, struct type *elem_type,
		      long long size)
{
	size_t i;
	struct type *t;

	for (i = 0; i < sc->nr_arr; i++) {
		struct type *arr = sc->arrs[i];
		if (arr->arr.elem_type == elem_type && arr->arr.size == size)
			return sc->arrs[i];
	}

	t = arena_alloc(&sc->cc->arena, sizeof(*t));
	memset(t, 0, sizeof(*t));

	t->kind = TY_ARR;
	t->arr.elem_type = elem_type;
	t->arr.size = size;

	ARENA_ALLOC_GROW(&sc->cc->arena, sc->arrs, sc->nr_arr + 1, sc->alloc_arr);
	sc->arrs[sc->nr_arr++] = t;
	return t;
}

struct type *type_tuple(struct semantic_context *sc, struct type **elems, size_t nr)
{
	size_t i;
	struct type *t, **e;

	for (i = 0; i < sc->nr_tuple; i++) {
		struct type *tp = sc->tuples[i];
		if (tp->tuple.nr == nr) {
			size_t j;
			for (j = 0; j < nr; j++)
				if (tp->tuple.elems[j] != elems[j])
					break;
			if (j == nr)
				return tp;
		}
	}

	t = arena_alloc(&sc->cc->arena, sizeof(*t));
	memset(t, 0, sizeof(*t));

	t->kind = TY_TUPLE;
	t->tuple.nr = nr;

	e = arena_alloc(&sc->cc->arena, sizeof(*e) * nr);
	memcpy(e, elems, sizeof(*e) * nr);
	t->tuple.elems = e;

	ARENA_ALLOC_GROW(&sc->cc->arena, sc->tuples, sc->nr_tuple + 1, sc->alloc_tuple);
	sc->tuples[sc->nr_tuple++] = t;
	return t;
}

struct type *type_fn(struct semantic_context *sc, struct type **params,
		     size_t nr, struct type *ret, unsigned variadic)
{
	size_t i;
	struct type *t, **p;

	for (i = 0; i < sc->nr_fn; i++) {
		struct type *fn = sc->fns[i];
		if (fn->fn.is_variadic == variadic && fn->fn.nr == nr &&
		    fn->fn.ret == ret) {
			size_t j;
			for (j = 0; j < nr; j++)
				if (fn->fn.params[j] != params[j])
					break;
			if (j == nr)
				return fn;
		}
	}

	t = arena_alloc(&sc->cc->arena, sizeof(*t));
	memset(t, 0, sizeof(*t));

	t->kind = TY_FN;
	t->fn.is_variadic = variadic;
	t->fn.nr = nr;
	t->fn.ret = ret;

	p = arena_alloc(&sc->cc->arena, sizeof(*t) * nr);
	memcpy(p, params, sizeof(*t) * nr);
	t->fn.params = p;

	ARENA_ALLOC_GROW(&sc->cc->arena, sc->fns, sc->nr_fn + 1, sc->alloc_fn);
	sc->fns[sc->nr_fn++] = t;
	return t;
}

// static struct symbol *scope_lookup(struct scope *sc, const char *id, size_t len)
// {
// 	struct scope *s;

// 	for (s = sc; s; s = s->parent) {
// 		size_t i;
// 		for (i = 0; i < s->nr; i++) {
// 			struct symbol *sym = s->symbols[i];
// 			if (sym->tok.len == len && !memcmp(sym->tok.lex, id, len))
// 				return sym;
// 		}
// 	}
// 	return NULL;
// }

static struct symbol *scope_lookup_local(struct scope *sc, const char *id, size_t len)
{
	size_t i;

	for (i = 0; i < sc->nr; i++) {
		struct symbol *sym = sc->symbols[i];
		if (sym->tok.len == len && !memcmp(sym->tok.lex, id, len))
			return sym;
	}
	return NULL;
}

static struct symbol *sym_new(struct semantic_context *sc, enum symbol_type kind,
			      struct token tok, struct ast_node *node)
{
	struct symbol *sym = arena_alloc(&sc->cc->arena, sizeof(*sym));
	memset(sym, 0, sizeof(*sym));
	sym->kind = kind;
	sym->tok = tok;
	sym->node = node;
	return sym;
}

static struct source_location loc_from_token(struct semantic_context *sc,
					     struct token tok)
{
	return (struct source_location){
		.file = sc->cc->filename,
		.line_start = tok.lex - tok.col,
		.line = tok.line,
		.col = tok.col,
		.len = (int)tok.len,
	};
}

static int scope_insert(struct semantic_context *sc, struct scope *s,
			struct symbol *sym)
{
	if (scope_lookup_local(s, sym->tok.lex, sym->tok.len)) {
		diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, sym->tok),
			  "'%.*s' has been already declared",
			  (int)sym->tok.len, sym->tok.lex);
		return 0;
	}

	ARENA_ALLOC_GROW(&sc->cc->arena, s->symbols, s->nr + 1, s->alloc);
	s->symbols[s->nr++] = sym;
	return 1;
}

static void hoist_top(struct semantic_context *sc, struct ast_node *program)
{
	size_t i;

	for (i = 0; i < program->block.nr; i++) {
		struct ast_node *node = program->block.childs[i];
		struct symbol *sym;

		switch (node->type) {
		case NODE_FN_DEC:
			sym = sym_new(sc, SYM_FN, node->tok, node);
			scope_insert(sc, sc->global, sym);
			node->rsym = sym;
			break;

		case NODE_STRUCT_DEC:
			sym = sym_new(sc, SYM_STRUCT, node->tok, node);
			scope_insert(sc, sc->global, sym);
			node->rsym = sym;
			break;

		case NODE_ENUM_DEC:
			sym = sym_new(sc, SYM_ENUM, node->tok, node);
			scope_insert(sc, sc->global, sym);
			node->rsym = sym;
			break;

		case NODE_TYPE_DEC:
			sym = sym_new(sc, SYM_TYPE_ALIAS, node->tok, node);
			scope_insert(sc, sc->global, sym);
			node->rsym = sym;
			break;

		case NODE_CONST_DEC:
			sym = sym_new(sc, SYM_CONST, node->tok, node);
			scope_insert(sc, sc->global, sym);
			node->rsym = sym;
			break;

		case NODE_LET_DEC:
			if (node->let_dec.nr_name == 1) {
				sym = sym_new(sc, SYM_VAR,
					      node->let_dec.name[0], node);
				sym->var.is_mut = 1;
				scope_insert(sc, sc->global, sym);
				node->rsym = sym;
			} else {
				size_t j;
				for (j = 0; j < node->let_dec.nr_name; j++) {
					sym = sym_new(sc, SYM_VAR,
						      node->let_dec.name[j],
						      node);
					sym->var.is_mut = 1;
					scope_insert(sc, sc->global, sym);
				}
			}
			break;
		default:
			break;
		}
	}
}

void semantic_analyze(struct semantic_context *sc, struct ast_node *program)
{
	hoist_top(sc, program);

	if (diag_has_errors(&sc->cc->diag))
		return;
}
