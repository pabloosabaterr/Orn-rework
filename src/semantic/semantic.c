#include "semantic/semantic.h"
#include "diagnostic/diagnostic.h"
#include "lexer/lexer.h"
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
static struct type *check_expr(struct semantic_context *sc, struct ast_node *node);
static void check_stmt(struct semantic_context *sc, struct ast_node *node);

static const char *type_name(struct type *t)
{
	switch (t->kind) {
	case TY_INT:
		return "int";
	case TY_UINT:
		return "unsigned";
	case TY_FLOAT:
		return "float";
	case TY_DOUBLE:
		return "double";
	case TY_BOOL:
		return "bool";
	case TY_CHAR:
		return "char";
	case TY_STRING:
		return "string";
	case TY_VOID:
		return "void";
	case TY_NULL:
		return "null";
	case TY_ERROR:
		return "<error>";
	case TY_PTR:
		return "pointer";
	case TY_ARR:
		return "array";
	case TY_TUPLE:
		return "tuple";
	case TY_FN:
		return "function";
	case TY_STRUCT:
		return "struct";
	case TY_ENUM:
		return "enum";
	case TY_TYPE_ALIAS:
		return "alias";
	}
	return "<unknown>";
}

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

static void scope_push(struct semantic_context *sc)
{
	struct scope *s = scope_new(sc, sc->current);
	sc->current = s;
}

static void scope_pop(struct semantic_context *sc)
{
	sc->current = sc->current->parent;
}

struct type *type_unalias(struct type *t)
{
	while (t && t->kind == TY_TYPE_ALIAS)
		t = t->alias.resolved;
	return t;
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

/*
 * NEEDSWORK: Once there are modules and imports become an actual thing. They
 * should be treated here.
 */
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

/*
 * NEEDSWORK: On patterns it should add the pattern parameters to the scope
 * so it can be used in the body.
 */
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

static int is_lvalue(struct ast_node *node)
{
	if (!node)
		return 0;

	switch (node->type) {
	case NODE_ID:
	case NODE_MEMBER:
	case NODE_INDEX:
		return 1;
	case NODE_UNARY:
		return node->unary.type == OP_DEREF;
	default:
		return 0;
	}
}

/*
 * NEEDSWORK: On functions, it is checked if the return, returns the correct
 * type but it is not check for whole function flow if every path has a return.
 * (This is specially needed for non-void functions)
 */
static void check_fn_body(struct semantic_context *sc, struct ast_node *node)
{
	struct symbol *sym = node->rsym;
	size_t i;

	sc->current_fn = sym;
	scope_push(sc);

	for (i = 0; i < sym->fn.nr_param; i++)
		scope_insert(sc, sc->current, sym->fn.params[i]);

	if (node->fn_dec.body)
		check_stmt(sc, node->fn_dec.body);

	scope_pop(sc);
	sc->current_fn = NULL;
}

static void check_bodies(struct semantic_context *sc, struct ast_node *program)
{
	size_t i;

	for (i = 0; i < program->block.nr; i++) {
		struct ast_node *node = program->block.childs[i];

		switch (node->type) {
		case NODE_FN_DEC:
			check_fn_body(sc, node);
			break;
		case NODE_IMPL_DEC: {
			size_t j;
			for (j = 0; j < node->list.nr_item; j++)
				check_fn_body(sc, node->list.items[j]);
			break;
		}
		case NODE_LET_DEC:
			check_stmt(sc, node);
			break;
		case NODE_EXPR_STMT:
		case NODE_IF:
		case NODE_WHILE:
		case NODE_FOR:
			check_stmt(sc, node);
			break;
		default:
			break;
		}
	}
}

static struct type *check_id(struct semantic_context *sc, struct ast_node *node)
{
	struct symbol *sym = scope_lookup(sc->current, node->tok.lex, node->tok.len);
	if (!sym) {
		diag_emit(&sc->cc->diag, ERROR,
			  loc_from_token(sc, node->tok),
			  "undeclared identifier '%.*s'",
			  (int)node->tok.len, node->tok.lex);
		return sc->t_err;
	}

	node->rsym = sym;

	if (!sym->type)
		return sc->t_err;

	return sym->type;
}

static int is_num(struct type *t)
{
	return t->kind == TY_INT || t->kind == TY_UINT || t->kind == TY_FLOAT ||
	       t->kind == TY_DOUBLE;
}

static int is_integer(struct type *t)
{
	return t->kind == TY_INT || t->kind == TY_UINT;
}

static struct type *number_promote(struct type *a, struct type *b)
{
	static const int rank[] = {
		[TY_INT] = 1,
		[TY_UINT] = 2,
		[TY_FLOAT] = 3,
		[TY_DOUBLE] = 4
	};

	if (!is_num(a) || !is_num(b))
		return NULL;

	if (a == b)
		return a;

	if (rank[a->kind] > rank[b->kind])
		return a;
	return b;
}

static int booleable(struct type *t)
{
	return t->kind == TY_BOOL || t->kind == TY_PTR;
}

static struct type *check_binary(struct semantic_context *sc, struct ast_node *node)
{
	struct type *left = check_expr(sc, node->binary.left);
	struct type *right = check_expr(sc, node->binary.right);
	struct type *promoted;

	if (left == sc->t_err || right == sc->t_err)
		return sc->t_err;

	switch (node->binary.type) {
	case OP_ADD:
		if (left->kind == TY_PTR && is_integer(right))
			return left;
		if (is_integer(left) && right->kind == TY_PTR)
			return right;

		promoted = number_promote(left, right);
		if (!promoted)
			goto bad_operands;
		return promoted;
	case OP_SUB:
		if (left->kind == TY_PTR && is_integer(right))
			return left;
		if (left->kind == TY_PTR && right->kind == TY_PTR) {
			if (left != right)
				goto bad_operands;
			return sc->t_int;
		}

		promoted = number_promote(left, right);
		if (!promoted)
			goto bad_operands;
		return promoted;
	case OP_MUL:
	case OP_DIV:
		promoted = number_promote(left, right);
		if (!promoted)
			goto bad_operands;
		return promoted;
	case OP_MOD:
		if (!is_integer(left) || !is_integer(right))
			goto bad_operands;
		promoted = number_promote(left, right);
		if (!promoted)
			goto bad_operands;
		return promoted;
	case OP_EQ:
	case OP_NEQ:
		if (left == right)
			return sc->t_bool;

		if (number_promote(left, right))
			return sc->t_bool;

		if ((left->kind == TY_PTR && right->kind == TY_NULL) ||
		    (left->kind == TY_NULL && right->kind == TY_PTR))
			return sc->t_bool;

		diag_emit(&sc->cc->diag, ERROR,
			  loc_from_token(sc, node->tok),
			  "cannot compare '%s' and '%s'",
			  type_name(left), type_name(right));
		return sc->t_err;
	case OP_LT:
	case OP_GT:
	case OP_LE:
	case OP_GE:
		if (left->kind == TY_PTR && left == right)
			return sc->t_bool;

		promoted = number_promote(left, right);
		if (!promoted) {
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, node->tok),
				  "cannot compare '%s' and '%s'",
				  type_name(left), type_name(right));
			return sc->t_err;
		}
		return sc->t_bool;
	case OP_AND:
	case OP_OR:
		if (!booleable(left) || !booleable(right)) {
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, node->tok),
				  "'%s' requires booleable operands",
				  op_name(node->binary.type));
			return sc->t_err;
		}
		return sc->t_bool;
	case OP_BIT_AND:
	case OP_BIT_OR:
	case OP_BIT_XOR:
		if (!is_integer(left) || !is_integer(right))
			goto bad_operands;
		promoted = number_promote(left, right);
		if (!promoted)
			goto bad_operands;
		return promoted;
	case OP_LSHIFT:
	case OP_RSHIFT:
		if (!is_integer(left) || !is_integer(right))
			goto bad_operands;
		return left;
	case OP_RANGE:
		if (!is_integer(left) || !is_integer(right))
			goto bad_operands;
		return left;

	default:
		goto bad_operands;
	}

bad_operands:
	diag_emit(&sc->cc->diag, ERROR,
		  loc_from_token(sc, node->tok),
		  "invalid operands to '%s': '%s' and '%s'",
		  op_name(node->binary.type),
		  type_name(left), type_name(right));
	return sc->t_err;
}

static struct type *check_unary(struct semantic_context *sc, struct ast_node *node)
{
	struct type *operand = check_expr(sc, node->unary.operand);

	if (operand == sc->t_err)
		return sc->t_err;

	switch (node->unary.type) {
	case OP_NEG:
		if (!is_num(operand)) {
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, node->tok),
				  "cannot negate '%s'", type_name(operand));
			return sc->t_err;
		}
		return operand;
	case OP_NOT:
		if (!booleable(operand)) {
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, node->tok),
				  "'!' requires 'bool' or pointer operand");
			return sc->t_err;
		}
		return sc->t_bool;
	case OP_BIT_NOT:
		if (!is_integer(operand)) {
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, node->tok),
				  "'~' requires integer operand");
			return sc->t_err;
		}
		return operand;
	case OP_ADDR:
		if (!is_lvalue(node->unary.operand)) {
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, node->tok),
				  "cannot take address of this expression");
			return sc->t_err;
		}
		return type_ptr(sc, operand);
	case OP_DEREF:
		if (operand->kind != TY_PTR) {
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, node->tok),
				  "cannot dereference '%s'",
				  type_name(operand));
			return sc->t_err;
		}
		return operand->ptr.pointee;
	default:
		return sc->t_err;
	}
}

static struct type *check_call(struct semantic_context *sc, struct ast_node *node)
{
	struct type *callee = check_expr(sc, node->call.callee);
	size_t i;

	if (callee == sc->t_err)
		return sc->t_err;

	if (callee->kind != TY_FN) {
		diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
			  "calling non-function");
		return sc->t_err;
	}

	if (node->call.nr_arg < callee->fn.nr ||
	    (!callee->fn.is_variadic && node->call.nr_arg > callee->fn.nr)) {
		diag_emit(&sc->cc->diag, ERROR,
			  loc_from_token(sc, node->tok),
			  "expected %zu arguments but got %zu",
			  callee->fn.nr, node->call.nr_arg);
		return sc->t_err;
	}

	for (i = 0; i < node->call.nr_arg; i++) {
		struct type *arg = check_expr(sc, node->call.args[i]);
		if (arg == sc->t_err)
			continue;

		if (i < callee->fn.nr && arg != callee->fn.params[i])
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, node->call.args[i]->tok),
				  "argument %zu: expected '%s' but got '%s'",
				  i + 1, type_name(callee->fn.params[i]),
				  type_name(arg));
	}

	return callee->fn.ret;
}

static struct type *check_assign(struct semantic_context *sc, struct ast_node *node)
{
	struct type *target = check_expr(sc, node->assign.target);
	struct type *val = check_expr(sc, node->assign.val);

	if (target == sc->t_err || val == sc->t_err)
		return sc->t_err;

	if (!is_lvalue(node->assign.target)) {
		diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
			  "cannot assign to this expression");
		return sc->t_err;
	}

	if (node->assign.type != OP_ASSIGN) {
		if (!is_num(target) || target != val) {
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, node->tok),
				  "invalid operands to compound assignment");
			return sc->t_err;
		}
		return target;
	}

	if (node->assign.target->type == NODE_ID &&
	    node->assign.target->rsym &&
	    !node->assign.target->rsym->var.is_mut) {
		diag_emit(&sc->cc->diag, ERROR,
			  loc_from_token(sc, node->tok),
			  "cannot assign to immutable variable '%.*s'",
			  (int)node->assign.target->rsym->tok.len,
			  node->assign.target->rsym->tok.lex);
		return sc->t_err;
	}

	if (target != val) {
		diag_emit(&sc->cc->diag, ERROR,
			  loc_from_token(sc, node->tok),
			  "cannot assign '%s' to '%s'",
			  type_name(val), type_name(target));
		return sc->t_err;
	}

	return target;
}

static struct type *check_cast(struct semantic_context *sc, struct ast_node *node)
{
	struct type *expr = check_expr(sc, node->cast.expr);
	struct type *target = resolve_type(sc, node->cast.target_type);

	if (expr == sc->t_err || target == sc->t_err)
		return sc->t_err;

	if ((is_num(expr) && is_num(target)) ||
	    (is_integer(expr) && target->kind == TY_CHAR) ||
	    (expr->kind == TY_CHAR && is_integer(target)) ||
	    (is_integer(expr) && target->kind == TY_PTR) ||
	    (expr->kind == TY_PTR && is_integer(target)))
		return target;

	diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
		  "cannot cast '%s' to '%s'", type_name(expr),
		  type_name(target));
	return sc->t_err;
}

static struct type *check_index(struct semantic_context *sc, struct ast_node *node)
{
	struct type *obj = check_expr(sc, node->index.obj);
	struct type *idx = check_expr(sc, node->index.idx);

	if (obj == sc->t_err)
		return sc->t_err;

	if (obj->kind != TY_ARR && obj->kind != TY_PTR) {
		diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
			  "cannot index '%s'", type_name(obj));
		return sc->t_err;
	}

	if (idx != sc->t_err && !is_integer(idx)) {
		diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
			  "index must be integer");
		return sc->t_err;
	}

	if (node->index.end) {
		struct type *end = check_expr(sc, node->index.end);
		if (end != sc->t_err && !is_integer(end))
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, node->tok),
				  "slice end must be integer");

		if (obj->kind == TY_ARR)
			return type_ptr(sc, obj->arr.elem_type);
		return obj;
	}

	if (obj->kind == TY_ARR)
		return obj->arr.elem_type;
	return obj->ptr.pointee;
}

static struct type *check_member(struct semantic_context *sc, struct ast_node *node)
{
	struct type *left = check_expr(sc, node->member.left);
	struct type *resolved;
	struct symbol *sym;
	size_t i;

	if (left == sc->t_err)
		return sc->t_err;

	resolved = type_unalias(left);

	if (resolved->kind == TY_TUPLE) {
		if (node->tok.type != TK_NUMBER) {
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, node->tok),
				  "tuple field must be a number");
			return sc->t_err;
		}
		long long idx = xstrtoll(node->tok.lex, node->tok.len, 10);
		if (idx < 0 || (size_t)idx >= resolved->tuple.nr) {
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, node->tok),
				  "tuple index out of bounds");
			return sc->t_err;
		}
		return resolved->tuple.elems[idx];
	}

	sym = resolved->named.dec;
	for (i = 0; i < sym->aggregate.nr; i++) {
		struct symbol *f = sym->aggregate.members[i];
		if (f->kind == SYM_STRUCT_FIELD && f->tok.len == node->tok.len &&
		    !memcmp(f->tok.lex, node->tok.lex, f->tok.len)) {
			node->rsym = f;
			return f->type;
		}
	}

	diag_emit(&sc->cc->diag, ERROR,
		  loc_from_token(sc, node->tok),
		  "no field '%.*s' in '%.*s'",
		  (int)node->tok.len, node->tok.lex,
		  (int)sym->tok.len, sym->tok.lex);
	return sc->t_err;
}

/*
 * NEEDSWORK: Now with a syntax like:
 *
 * let c: Color = ::Red;
 *
 * Red its being searched thorugh all the scopes for an obj that matches that
 * same member when it should look the type : Color before instead of blindly
 * searching through all scopes, symbols and obj members.
 *
 * With the current approach there is this limitation where even if the type
 * is set, if two enums happen to have same member name, there is no way of
 * knowing which one the user has set to be. The temporal solution is to write
 * the full namespace Color::Red instead of the abreviation.
 *
 * Two possible solutions thought are:
 *
 * - When a declaration expects an enum member, propagate the enum symbol down
 *   up to this function which will use it instead of searching. O(1) ?
 *
 * - Same but instead of propagating at the context have somehting like
 *   'expected type' which is set at declarations that expects enums members to
 *   later be used here. O(1) ?
 */
static struct type *check_namespace(struct semantic_context *sc, struct ast_node *node)
{
	struct symbol *target;
	struct symbol *member;
	size_t i;

	/*
	 * Color::Red
	 * node->namespace.left (Color), can be null on ::Red
	 * node->tok the token information is at Red
	 */
	if (!node->namespace.left) {
		struct scope *s;
		struct symbol *found = NULL;
		struct symbol *found_parent = NULL;
		int count = 0;

		/* NEEDSWORK: O(n3)... */
		for (s = sc->current; s; s = s->parent) {
			size_t j;
			for (j = 0; j < s->nr; j++) {
				struct symbol *sym = s->symbols[j];
				size_t k;
				if (sym->kind != SYM_ENUM)
					continue;
				for (k = 0; k < sym->aggregate.nr; k++) {
					struct symbol *m = sym->aggregate.members[k];
					if (m->kind != SYM_ENUM_MEMBER)
						continue;
					if (m->tok.len == node->tok.len &&
					    !memcmp(m->tok.lex, node->tok.lex,
						    m->tok.len)) {
						found = m;
						found_parent = sym;
						count++;
					}
				}
			}
		}
		if (count == 0) {
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, node->tok),
				  "unknown enum member '%.*s'",
				  (int)node->tok.len, node->tok.lex);
			return sc->t_err;
		}
		if (count > 1) {
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, node->tok),
				  "ambiguous '::': '%.*s' exists in multiple enums",
				  (int)node->tok.len, node->tok.lex);
			return sc->t_err;
		}

		node->rsym = found;
		return found_parent->type;
	}

	target = scope_lookup(sc->current, node->namespace.left->tok.lex,
			      node->namespace.left->tok.len);

	if (!target) {
		diag_emit(&sc->cc->diag, ERROR,
			  loc_from_token(sc, node->namespace.left->tok),
			  "unknown type '%.*s'",
			  (int)node->namespace.left->tok.len,
			  node->namespace.left->tok.lex);
		return sc->t_err;
	}

	if (target->kind != SYM_STRUCT && target->kind != SYM_ENUM) {
		diag_emit(&sc->cc->diag, ERROR,
			  loc_from_token(sc, node->namespace.left->tok),
			  "'%.*s' has no namespace members",
			  (int)target->tok.len, target->tok.lex);
		return sc->t_err;
	}

	for (i = 0; i < target->aggregate.nr; i++) {
		member = target->aggregate.members[i];
		if (member->tok.len == node->tok.len &&
		    !memcmp(member->tok.lex, node->tok.lex,
			    member->tok.len)) {
			node->rsym = member;
			if (member->kind == SYM_ENUM_MEMBER)
				return target->type;
			if (member->kind == SYM_FN)
				return member->type;
			return member->type;
		}
	}

	diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
		  "'%.*s' has no member '%.*s'",
		  (int)target->tok.len, target->tok.lex,
		  (int)node->tok.len, node->tok.lex);
	return sc->t_err;
}

static struct type *check_struct_init(struct semantic_context *sc, struct ast_node *node)
{
	struct symbol *sym;
	size_t i;

	sym = scope_lookup(sc->current, node->tok.lex, node->tok.len);
	if (!sym || sym->kind != SYM_STRUCT) {
		diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
			  "'%.*s' is not a struct",
			  (int)node->tok.len, node->tok.lex);
		return sc->t_err;
	}

	node->rsym = sym;

	for (i = 0; i < node->list.nr_item; i++) {
		struct ast_node *fi = node->list.items[i];
		struct symbol *field = NULL;
		struct type *val_type;
		size_t j;

		for (j = 0; j < i; j++) {
			struct ast_node *prev = node->list.items[j];
			if (prev->tok.len == fi->tok.len &&
			    !memcmp(prev->tok.lex, fi->tok.lex, prev->tok.len)) {
				diag_emit(&sc->cc->diag, ERROR,
					  loc_from_token(sc, fi->tok),
					  "duplicate field '%.*s' in initializer",
					  (int)fi->tok.len, fi->tok.lex);
				break;
			}
		}

		for (j = 0; j < sym->aggregate.nr; j++) {
			struct symbol *f = sym->aggregate.members[j];
			if (f->kind == SYM_STRUCT_FIELD &&
			    f->tok.len == fi->tok.len &&
			    !memcmp(f->tok.lex, fi->tok.lex, f->tok.len)) {
				field = f;
				break;
			}
		}

		if (!field) {
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, fi->tok),
				  "no field '%.*s' in '%.*s'",
				  (int)fi->tok.len, fi->tok.lex,
				  (int)sym->tok.len, sym->tok.lex);
			continue;
		}

		fi->rsym = field;
		val_type = check_expr(sc, fi->field_init.val);

		if (val_type != sc->t_err && val_type != field->type)
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, fi->tok),
				  "field '%.*s': expected '%s' but got '%s'",
				  (int)fi->tok.len, fi->tok.lex,
				  type_name(field->type),
				  type_name(val_type));
	}

	for (i = 0; i < sym->aggregate.nr; i++) {
		struct symbol *member = sym->aggregate.members[i];
		size_t j;
		int found = 0;

		if (member->kind != SYM_STRUCT_FIELD)
			continue;

		for (j = 0; j < node->list.nr_item; j++) {
			struct ast_node *field = node->list.items[j];
			if (field->tok.len == member->tok.len &&
			    !memcmp(field->tok.lex, member->tok.lex, field->tok.len)) {
				found = 1;
				break;
			}
		}

		if (!found)
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, node->tok),
				  "missing field '%.*s' in '%.*s'",
				  (int)member->tok.len, member->tok.lex,
				  (int)sym->tok.len, sym->tok.lex);
	}

	return sym->type;
}

static struct type *check_array_init(struct semantic_context *sc, struct ast_node *node)
{
	struct type *elem = NULL;
	size_t i;

	if (node->list.nr_item == 0) {
		diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
			  "empty array literal");
		return sc->t_err;
	}

	for (i = 0; i < node->list.nr_item; i++) {
		struct type *t = check_expr(sc, node->list.items[i]);
		if (t == sc->t_err)
			continue;
		if (!elem)
			elem = t;
		else if (t != elem)
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, node->list.items[i]->tok),
				  "array element type mismatch: "
				  "expected '%s' but got '%s'",
				  type_name(elem), type_name(t));
	}

	if (!elem)
		return sc->t_err;

	return type_arr(sc, elem, (long long)node->list.nr_item);
}

static void check_return(struct semantic_context *sc, struct ast_node *node)
{
	struct type *ret;

	if (!sc->current_fn) {
		diag_emit(&sc->cc->diag, ERROR,
			  loc_from_token(sc, node->tok),
			  "'return' outside of function");
		return;
	}

	if (sc->in_defer) {
		diag_emit(&sc->cc->diag, ERROR,
			  loc_from_token(sc, node->tok),
			  "'return' inside 'defer'");
		return;
	}

	struct type *expected = sc->current_fn->type->fn.ret;

	if (node->return_stmt.expr) {
		ret = check_expr(sc, node->return_stmt.expr);
		if (ret != sc->t_err && ret != expected)
			diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
				  "return type mismatch: expected '%s' "
				  "but got '%s'",
				  type_name(expected),
				  type_name(ret));
	} else {
		if (expected != sc->t_void)
			diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
				  "non-void function must return a value");
	}
}

static int type_compatible(struct type *target, struct type *value)
{
	if (target == value)
		return 1;

	if (target->kind == TY_PTR && value->kind == TY_NULL)
		return 1;

	if (number_promote(target, value))
		return 1;

	if (type_unalias(target) == type_unalias(value))
		return 1;

	return 0;
}

static void check_let(struct semantic_context *sc, struct ast_node *node)
{
	struct type *ann = NULL;
	struct type *init = NULL;

	if (node->let_dec.ann)
		ann = resolve_type(sc, node->let_dec.ann);
	if (node->let_dec.init)
		init = check_expr(sc, node->let_dec.init);

	if (node->let_dec.nr_name == 1) {
		struct symbol *sym;

		sym = scope_lookup_local(sc->current, node->let_dec.name[0].lex,
					 node->let_dec.name[0].len);

		if (!sym) {
			sym = sym_new(sc, SYM_VAR, node->let_dec.name[0], node);
			sym->var.is_mut = 1;
			scope_insert(sc, sc->current, sym);
			node->rsym = sym;
		}

		if (ann && init && init != sc->t_err) {
			if (ann->kind == TY_ARR && init->kind == TY_ARR &&
			    ann->arr.size != init->arr.size)
				diag_emit(&sc->cc->diag, ERROR,
					  loc_from_token(sc, node->tok),
					  "array size mismatch: expected %lld but got %lld",
					  ann->arr.size, init->arr.size);
			else if (!type_compatible(ann, init))
				diag_emit(&sc->cc->diag, ERROR,
					  loc_from_token(sc, node->tok),
					  "type mismatch: '%s' vs '%s'",
					  type_name(ann), type_name(init));
		}

		sym->type = ann ? ann : init;
		sym->var.is_init = (init != NULL);
		return;
	}

	if (init && init != sc->t_err) {
		struct type *resolved = type_unalias(init);
		if (resolved->kind != TY_TUPLE) {
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, node->tok),
				  "destructuring requires tuple, got '%s'",
				  type_name(init));
			return;
		}
		if (resolved->tuple.nr != node->let_dec.nr_name) {
			diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
				  "expected %zu names but got %zu elements",
				  node->let_dec.nr_name, resolved->tuple.nr);
			return;
		}

		size_t i;
		for (i = 0; i < node->let_dec.nr_name; i++) {
			struct symbol *sym;
			sym = sym_new(sc, SYM_VAR, node->let_dec.name[i], node);
			sym->var.is_mut = 1;
			sym->type = resolved->tuple.elems[i];
			sym->var.is_init = 1;
			scope_insert(sc, sc->current, sym);
		}
	}
}

static void check_if(struct semantic_context *sc, struct ast_node *node)
{
	size_t i;

	for (i = 0; i < node->if_stmt.nr_branch; i++) {
		struct type *cond = check_expr(sc, node->if_stmt.conds[i]);
		if (cond != sc->t_err && !booleable(cond))
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, node->if_stmt.conds[i]->tok),
				  "condition must be a booleable type, got '%s'",
				  type_name(cond));
		check_stmt(sc, node->if_stmt.bodies[i]);
	}

	if (node->if_stmt.else_body)
		check_stmt(sc, node->if_stmt.else_body);
}

static void check_while(struct semantic_context *sc, struct ast_node *node)
{
	struct type *cond = check_expr(sc, node->while_stmt.cond);

	if (cond != sc->t_err && !booleable(cond))
		diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
			  "condition must be a booleable type, got '%s'",
			  type_name(cond));

	sc->loop_depth++;
	check_stmt(sc, node->while_stmt.body);
	sc->loop_depth--;
}

static void check_for(struct semantic_context *sc, struct ast_node *node)
{
	struct type *range = check_expr(sc, node->for_stmt.range);
	struct symbol *iter;

	scope_push(sc);

	iter = sym_new(sc, SYM_VAR, node->tok, node);
	iter->var.is_mut = 0;
	iter->var.is_init = 1;

	if (range != sc->t_err && is_integer(range))
		iter->type = range;
	else if (range != sc->t_err) {
		diag_emit(&sc->cc->diag, ERROR,
			  loc_from_token(sc, node->tok),
			  "'for' range must be integer range");
		iter->type = sc->t_err;
	} else {
		iter->type = sc->t_err;
	}

	scope_insert(sc, sc->current, iter);
	node->rsym = iter;

	sc->loop_depth++;
	check_stmt(sc, node->for_stmt.body);
	sc->loop_depth--;

	scope_pop(sc);
}

/*
 * NEEDSWORK: Check a match's without wildcards (a match where not all cases
 * are covered). It should throw atleast a warning? so the user knows that
 * it can fall through.
 */
static void check_match(struct semantic_context *sc, struct ast_node *node)
{
	struct type *subj = check_expr(sc, node->match.subj);
	size_t i;

	for (i = 0; i < node->match.nr_arm; i++) {
		struct ast_node *arm = node->match.arms[i];
		struct ast_node *pattern = arm->match_arm.pattern;

		if (!pattern->match_pattern.is_wildcard &&
		    pattern->match_pattern.expr) {
			struct type *pt = check_expr(sc, pattern->match_pattern.expr);
			if (pt != sc->t_err && subj != sc->t_err && pt != subj)
				diag_emit(&sc->cc->diag, ERROR,
					  loc_from_token(sc, pattern->tok),
					  "pattern type '%s' doesn't match "
					  "subject type '%s'",
					  type_name(pt), type_name(subj));
		}

		check_stmt(sc, arm->match_arm.body);
	}
}

static void check_incdec(struct semantic_context *sc, struct ast_node *node)
{
	struct type *t = check_expr(sc, node->incdec.target);

	if (!is_lvalue(node->incdec.target)) {
		diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
			  "cannot mutate this expression");
		return;
	}

	if (t != sc->t_err && !is_integer(t)) {
		diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
			  "'%s' requires integer operand",
			  node->incdec.type == OP_INC ? "++" : "--");
		return;
	}

	if (node->incdec.target->rsym && !node->incdec.target->rsym->var.is_mut) {
		diag_emit(&sc->cc->diag, ERROR,
			  loc_from_token(sc, node->tok),
			  "cannot mutate immutable variable '%.*s'",
			  (int)node->incdec.target->rsym->tok.len,
			  node->incdec.target->rsym->tok.lex);
		return;
	}
}

static void check_const(struct semantic_context *sc, struct ast_node *node)
{
	struct type *ann = resolve_type(sc, node->const_dec.ann);
	struct type *init = check_expr(sc, node->const_dec.init);

	if (ann != sc->t_err && init != sc->t_err && ann != init)
		diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
			  "const type mismatch: '%s' vs '%s'",
			  type_name(ann), type_name(init));

	/* local consts */
	if (!node->rsym) {
		struct symbol *sym = sym_new(sc, SYM_CONST, node->tok, node);
		sym->type = ann;
		scope_insert(sc, sc->current, sym);
		node->rsym = sym;
	} else {
		node->rsym->type = ann;
	}
}

static void check_stmt(struct semantic_context *sc, struct ast_node *node)
{
	if (!node)
		return;

	switch (node->type) {
	case NODE_BLOCK: {
		size_t i;
		scope_push(sc);
		for (i = 0; i < node->block.nr; i++)
			check_stmt(sc, node->block.childs[i]);
		scope_pop(sc);
		break;
	}
	case NODE_EXPR_STMT:
		check_expr(sc, node->expr_stmt.expr);
		break;
	case NODE_RETURN:
		check_return(sc, node);
		break;
	case NODE_LET_DEC:
		check_let(sc, node);
		break;
	case NODE_CONST_DEC:
		check_const(sc, node);
		break;
	case NODE_IF:
		check_if(sc, node);
		break;
	case NODE_WHILE:
		check_while(sc, node);
		break;
	case NODE_FOR:
		check_for(sc, node);
		break;
	case NODE_BREAK:
		if (sc->loop_depth == 0)
			diag_emit(&sc->cc->diag, ERROR, loc_from_token(sc, node->tok),
				  "'break' outside of loop");
		break;
	case NODE_CONTINUE:
		if (sc->loop_depth == 0)
			diag_emit(&sc->cc->diag, ERROR,
				  loc_from_token(sc, node->tok),
				  "'continue' outside of loop");
		break;
	case NODE_DEFER:
		sc->in_defer = 1;
		check_stmt(sc, node->defer_stmt.stmt);
		sc->in_defer = 0;
		break;
	case NODE_MATCH:
		check_match(sc, node);
		break;
	case NODE_INCDEC:
		check_incdec(sc, node);
		break;
	default:
		break;
	}
}

static struct type *check_expr(struct semantic_context *sc, struct ast_node *node)
{
	struct type *t;

	if (!node)
		return sc->t_err;

	switch (node->type) {
	case NODE_INT:
		t = sc->t_int;
		break;
	case NODE_FLOATING:
		t = sc->t_double;
		break;
	case NODE_BOOL:
		t = sc->t_bool;
		break;
	case NODE_CHAR:
		t = sc->t_char;
		break;
	case NODE_STRING:
		t = sc->t_string;
		break;
	case NODE_NULL:
		t = sc->t_null;
		break;
	case NODE_ID:
		t = check_id(sc, node);
		break;
	case NODE_BINARY:
		t = check_binary(sc, node);
		break;
	case NODE_UNARY:
		t = check_unary(sc, node);
		break;
	case NODE_CALL:
		t = check_call(sc, node);
		break;
	case NODE_ASSIGN:
		t = check_assign(sc, node);
		break;
	case NODE_CAST:
		t = check_cast(sc, node);
		break;
	case NODE_INDEX:
		t = check_index(sc, node);
		break;
	case NODE_MEMBER:
		t = check_member(sc, node);
		break;
	case NODE_NAMESPACE:
		t = check_namespace(sc, node);
		break;
	case NODE_SIZEOF:
		t = sc->t_int;
		break;
	case NODE_STRUCT_INIT:
		t = check_struct_init(sc, node);
		break;
	case NODE_ARRAY_INIT:
		t = check_array_init(sc, node);
		break;
	default:
		diag_emit(&sc->cc->diag, ERROR,
			  loc_from_token(sc, node->tok),
			  "unexpected expression");
		t = sc->t_err;
		break;
	}

	node->rtype = t;
	return t;
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

	check_bodies(sc, program);
}
