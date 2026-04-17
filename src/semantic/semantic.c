#include "semantic/semantic.h"
#include "diagnostic/diagnostic.h"
#include "memory/arena.h"
#include "compiler.h"
#include "parser/ast.h"
#include "semantic/type.h"
#include "memory/wrapper.h"
#include <stddef.h>
#include <string.h>

/*
 * used for three states resolved_state at struct symbol.
 */
enum {
	UNRESOLVED,
	RESOLVING,
	RESOLVED
};

static struct type *resolve_type(struct semantic_context *sc, struct ast_node *node);

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

static struct symbol *scope_lookup(struct scope *sc, const char *id, size_t len)
{
	struct scope *s;

	for (s = sc; s; s = s->parent) {
		size_t i;
		for (i = 0; i < s->nr; i++) {
			struct symbol *sym = s->symbols[i];
			if (sym->tok.len == len && !memcmp(sym->tok.lex, id, len))
				return sym;
		}
	}
	return NULL;
}

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

static struct type *resolve_alias(struct semantic_context *sc, struct symbol *sym)
{
	struct type *target;

	if (sym->resolved_state == RESOLVED)
		return sym->type;

	if (sym->resolved_state == RESOLVING) {
		diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, sym->tok),
			  "cyclic type alias '%.*s'", (int)sym->tok.len, sym->tok.lex);
		sym->type = sc->t_err;
		sym->resolved_state = RESOLVED;
		return sc->t_err;
	}

	sym->resolved_state = RESOLVING;
	target = resolve_type(sc, sym->node->type_dec.type);
	sym->type = target;
	sym->resolved_state = RESOLVED;
	return target;
}

static struct type *resolve_type_name(struct semantic_context *sc, struct ast_node *node)
{
	const char *name = node->tok.lex;
	size_t len = node->tok.len;
	struct symbol *sym;

	switch (node->tok.type) {
	case TK_INT:
		return sc->t_int;
	case TK_UINT:
		return sc->t_uint;
	case TK_FLOAT:
		return sc->t_float;
	case TK_DOUBLE:
		return sc->t_double;
	case TK_BOOL:
		return sc->t_bool;
	case TK_CHAR:
		return sc->t_char;
	case TK_STRING:
		return sc->t_string;
	case TK_VOID:
		return sc->t_void;
	case TK_NULL:
		return sc->t_null;
	default:
		break;
	}

	sym = scope_lookup(sc->current, name, len);
	if (!sym) {
		diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
			  "unknown type '%.*s'", (int)len, name);
		return sc->t_err;
	}

	switch (sym->kind) {
	case SYM_STRUCT:
	case SYM_ENUM:
		node->rsym = sym;
		return sym->type;
	case SYM_TYPE_ALIAS:
		node->rsym = sym;
		return resolve_alias(sc, sym);
	default:
		diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
			  "'%.*s' is not a type", (int)len, name);
		return sc->t_err;
	}
}

static struct type *resolve_type(struct semantic_context *sc, struct ast_node *node)
{
	struct type *inner;

	if (!node)
		return sc->t_err;

	switch (node->type) {
	case NODE_TYPE_NAME:
		return resolve_type_name(sc, node);
	case NODE_TYPE_ARR:
		inner = resolve_type(sc, node->type_array.elem_type);
		return type_arr(sc, inner, node->type_array.size);
	case NODE_TYPE_PTR:
		inner = resolve_type(sc, node->type_ptr.type);
		return type_ptr(sc, inner);
	case NODE_TYPE_TUPLE: {
		struct type **elems;
		size_t i;

		elems = arena_alloc(&sc->cc->arena, sizeof(*elems) * node->list.nr_item);
		for (i = 0; i < node->list.nr_item; i++)
			elems[i] = resolve_type(sc, node->list.items[i]);

		return type_tuple(sc, elems, node->list.nr_item);
	}
	default:
		diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok), "expected type");
		return sc->t_err;
	}
}

static void resolve_struct(struct semantic_context *sc, struct ast_node *node)
{
	struct symbol *sym = node->rsym;
	struct ast_node *field_node;
	struct symbol *field_sym;
	struct type *t;
	size_t i, nr;

	nr = node->list.nr_item;
	sym->aggregate.members = arena_alloc(&sc->cc->arena, sizeof(*sym->aggregate.members) * nr);
	sym->aggregate.nr = nr;
	sym->aggregate.alloc = nr;

	for (i = 0; i < nr; i++) {
		size_t j;

		field_node = node->list.items[i];
		for (j = 0; j < i; j++) {
			struct symbol *prev = sym->aggregate.members[j];
			if (prev->tok.len == field_node->tok.len &&
			    !memcmp(prev->tok.lex, field_node->tok.lex, prev->tok.len)) {
				diag_emit(&sc->cc->diag, ERROR,
					  loc_from_token(sc, field_node->tok),
					  "duplicated field '%.*s'",
					  (int)field_node->tok.len,
					  field_node->tok.lex);
				break;
			}
		}

		field_sym = sym_new(sc, SYM_STRUCT_FIELD, field_node->tok, field_node);
		field_sym->type = resolve_type(sc, field_node->typed.ann);
		field_sym->field.index = i;
		field_sym->field.parent = sym;

		sym->aggregate.members[i] = field_sym;
		field_node->rsym = field_sym;
	}

	t = arena_alloc(&sc->cc->arena, sizeof(*t));
	memset(t, 0, sizeof(*t));
	t->kind = TY_STRUCT;
	t->named.dec = sym;
	sym->type = t;
	sym->resolved_state = RESOLVED;
}

static void resolve_enum(struct semantic_context *sc, struct ast_node *node)
{
	struct symbol *sym = node->rsym;
	struct ast_node *member_node;
	struct symbol *member_symbol;
	struct type *t;
	size_t i, nr;
	long long next_val = 0;

	nr = node->list.nr_item;
	sym->aggregate.members = arena_alloc(&sc->cc->arena, sizeof(*sym->aggregate.members) * nr);
	sym->aggregate.nr = nr;
	sym->aggregate.alloc = nr;

	for (i = 0; i < nr; i++) {
		member_node = node->list.items[i];
		member_symbol = sym_new(sc, SYM_ENUM_MEMBER, member_node->tok, member_node);
		member_symbol->enum_member.parent = sym;

		if (member_node->enum_member.val) {
			struct ast_node *val = member_node->enum_member.val;
			if (val->type == NODE_INT) {
				member_symbol->enum_member.val = val->lit_int.val;
				member_symbol->enum_member.explicit = 1;
				next_val = val->lit_int.val + 1;
			} else {
				diag_emit(&sc->cc->diag, ERROR,
					  loc_from_token(sc, val->tok),
					  "enum value must be an integer constant");
				member_symbol->enum_member.val = next_val++;
			}
		} else {
			member_symbol->enum_member.val = next_val++;
		}

		if (member_node->enum_member.nr_assoc > 0) {
			size_t j;
			for (j = 0; j < member_node->enum_member.nr_assoc; j++) {
				struct ast_node *assoc = member_node->enum_member.assocs[j];
				struct type *at = resolve_type(sc, assoc);
				assoc->rtype = at;
			}
		}

		member_symbol->type = NULL;
		sym->aggregate.members[i] = member_symbol;
		member_node->rsym = member_symbol;
	}

	t = arena_alloc(&sc->cc->arena, sizeof(*t));
	memset(t, 0, sizeof(*t));
	t->kind = TY_ENUM;
	t->named.dec = sym;
	sym->type = t;
	sym->resolved_state = RESOLVED;
}

static void resolve_types(struct semantic_context *sc, struct ast_node *program)
{
	size_t i;

	for (i = 0; i < program->block.nr; i++) {
		struct ast_node *node = program->block.childs[i];

		switch (node->type) {
		case NODE_TYPE_DEC:
			resolve_alias(sc, node->rsym);
			break;
		case NODE_STRUCT_DEC:
			resolve_struct(sc, node);
			break;
		case NODE_ENUM_DEC:
			resolve_enum(sc, node);
			break;
		default:
			break;
		}
	}
}

static void resolve_fn_sig(struct semantic_context *sc, struct ast_node *node,
			   struct symbol *sym)
{
	struct type **param_types;
	size_t i, nr;

	nr = node->fn_dec.nr_param;

	sym->fn.params = arena_alloc(&sc->cc->arena, sizeof(*sym->fn.params) * nr);
	sym->fn.nr_param = nr;

	param_types = arena_alloc(&sc->cc->arena, sizeof(*sym->fn.params) * nr);

	for (i = 0; i < nr; i++) {
		struct ast_node *p = node->fn_dec.params[i];
		struct symbol *p_sym;

		p_sym = sym_new(sc, SYM_PARAM, p->tok, p);
		p_sym->type = resolve_type(sc, p->typed.ann);
		param_types[i] = p_sym->type;
		p->rsym = p_sym;

		sym->fn.params[i] = p_sym;
	}

	struct type *ret;
	if (node->fn_dec.ret_type)
		ret = resolve_type(sc, node->fn_dec.ret_type);
	else
		ret = sc->t_void;

	sym->type = type_fn(sc, param_types, nr, ret, node->fn_dec.is_variadic);
	sym->resolved_state = RESOLVED;
}

static void resolve_impl(struct semantic_context *sc, struct ast_node *node)
{
	struct symbol *target;
	size_t i;

	target = scope_lookup(sc->current, node->tok.lex, node->tok.len);
	if (!target) {
		diag_emit(&sc->cc->diag, ERROR,
			  loc_from_token(sc, node->tok),
			  "impl for unknown type '%.*s'",
			  (int)node->tok.len, node->tok.lex);
		return;
	}

	if (target->kind != SYM_STRUCT && target->kind != SYM_ENUM) {
		diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
			  "cannot impl '%.*s': not a struct or enum",
			  (int)node->tok.len, node->tok.lex);
		return;
	}

	for (i = 0; i < node->list.nr_item; i++) {
		struct ast_node *fn_node = node->list.items[i];
		struct symbol *fn_sym;
		size_t j;

		for (j = 0; j < target->aggregate.nr; j++) {
			struct symbol *prev = target->aggregate.members[j];
			if (prev->tok.len == fn_node->tok.len &&
			    !memcmp(prev->tok.lex, fn_node->tok.lex,
				    prev->tok.len)) {
				diag_emit(&sc->cc->diag, ERROR,
					  loc_from_token(sc, fn_node->tok),
					  "duplicate method '%.*s' in '%.*s'",
					  (int)fn_node->tok.len,
					  fn_node->tok.lex,
					  (int)node->tok.len,
					  node->tok.lex);
				break;
			}
		}

		fn_sym = sym_new(sc, SYM_FN, fn_node->tok, fn_node);
		resolve_fn_sig(sc, fn_node, fn_sym);
		fn_node->rsym = fn_sym;

		ARENA_ALLOC_GROW(&sc->cc->arena, target->aggregate.members,
				 target->aggregate.nr + 1, target->aggregate.alloc);
		target->aggregate.members[target->aggregate.nr++] = fn_sym;
	}
}

static void resolve_const(struct semantic_context *sc, struct ast_node *node)
{
	struct symbol *sym = node->rsym;

	sym->type = resolve_type(sc, node->const_dec.ann);
	sym->resolved_state = RESOLVED;
}

static void resolve_let(struct semantic_context *sc, struct ast_node *node)
{
	struct type *ann = NULL;
	size_t i;

	if (node->let_dec.ann)
		ann = resolve_type(sc, node->let_dec.ann);

	if (node->let_dec.nr_name == 1 && ann) {
		node->rsym->type = ann;
		return;
	}

	/* infering happens next stage */
	if (!ann)
		return;

	if (ann->kind != TY_TUPLE) {
		diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
			  "destructuring requires a tuple type");
		return;
	}

	for (i = 0; i < node->let_dec.nr_name; i++) {
		struct symbol *sym = scope_lookup_local(sc->current,
							node->let_dec.name[i].lex,
							node->let_dec.name[i].len);
		if (sym)
			sym->type = ann->tuple.elems[i];
	}
}

static void resolve_sig(struct semantic_context *sc, struct ast_node *program)
{
	size_t i;

	for (i = 0; i < program->block.nr; i++) {
		struct ast_node *node = program->block.childs[i];

		switch (node->type) {
		case NODE_FN_DEC:
			resolve_fn_sig(sc, node, node->rsym);
			break;
		case NODE_IMPL_DEC:
			resolve_impl(sc, node);
			break;
		case NODE_CONST_DEC:
			resolve_const(sc, node);
			break;
		case NODE_LET_DEC:
			resolve_let(sc, node);
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

	resolve_types(sc, program);
	if (diag_has_errors(&sc->cc->diag))
		return;

	resolve_sig(sc, program);
	if (diag_has_errors(&sc->cc->diag))
		return;
}
