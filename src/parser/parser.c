#include "parser.h"
#include "lexer/lexer.h"
#include "memory/arena.h"
#include "memory/wrapper.h"
#include "parser/ast.h"

#include <stddef.h>
#include <string.h>

static struct ast_node *parse_expr(struct parser_context *p);
static struct ast_node *parse_range(struct parser_context *p);
static struct ast_node *parse_unary(struct parser_context *p);
static struct ast_node *parse_stmt(struct parser_context *p);
static struct token *parse_id_list(struct parser_context *p, size_t *nr);

static struct token advance(struct parser_context *p)
{
	p->prev = p->current;
	p->current = token_next(p->lexer);
	return p->prev;
}

static inline int check(struct parser_context *p, enum token_type type)
{
	return p->current.type == type;
}

static int match(struct parser_context *p, enum token_type type)
{
	if (!check(p, type))
		return 0;
	advance(p);
	return 1;
}

static void expect(struct parser_context *p, enum token_type type)
{
	if (!match(p, type))
		die("expected %d but got %d at %d:%d", type,
		    p->current.type, p->current.line,
		    p->current.col);
}

static struct ast_node *parse_type(struct parser_context *p)
{
	struct ast_node *node;
	struct token tok;

	/* Pointers */
	if (check(p, TK_STAR)) {
		tok = advance(p);
		node = ast_node_new(&p->arena, NODE_TYPE_PTR, tok);
		node->type_ptr.type = parse_type(p);
		return node;
	}

	/* Arrays */
	if (check(p, TK_LBRACKET)) {
		tok = advance(p);
		node = ast_node_new(&p->arena, NODE_TYPE_ARR, tok);

		node->type_array.elem_type = parse_type(p);

		expect(p, TK_SEMICOLON);
		expect(p, TK_NUMBER);
		node->type_array.size = xstrtoll(p->prev.lex, p->prev.len, 10);

		expect(p, TK_RBRACKET);
		return node;
	}

	/* Tuples */
	if (check(p, TK_LPAREN)) {
		size_t nr = 0, alloc = 0;
		tok = advance(p);
		node = ast_node_new(&p->arena, NODE_TYPE_TUPLE, tok);

		while (!check(p, TK_RPAREN)) {
			ARENA_ALLOC_GROW(&p->arena, node->list.items, nr + 1, alloc);
			node->list.items[nr++] = parse_type(p);
			if (!check(p, TK_RPAREN))
				expect(p, TK_COMMA);
		}

		expect(p, TK_RPAREN);
		node->list.nr_item = nr;

		return node;
	}

	tok = advance(p);
	switch (tok.type) {
	case TK_INT:
	case TK_UINT:
	case TK_FLOATING:
	case TK_STRING:
	case TK_CHAR:
	case TK_BOOL:
	case TK_NULL:
	case TK_VOID:
	case TK_ID:
		return ast_node_new(&p->arena, NODE_TYPE_NAME, tok);
	default:
		die("expected type at %d:%d", tok.line, tok.col);
	}
}

/*
 * Expressions
 */
static struct ast_node *parse_numbers(struct parser_context *p,
				      int base, int prefix_len)
{
	struct token tok = advance(p);
	struct ast_node *node = ast_node_new(&p->arena, NODE_INT, tok);
	node->lit_int.val = xstrtoll(tok.lex + prefix_len, tok.len - prefix_len,
				     base);
	return node;
}

static struct ast_node *parse_field_init(struct parser_context *p)
{
	struct token name = advance(p);
	struct ast_node *node;

	expect(p, TK_COLON);
	node = ast_node_new(&p->arena, NODE_FIELD_INIT, name);
	node->field_init.val = parse_expr(p);
	return node;
}

static struct ast_node *parse_primary(struct parser_context *p)
{
	struct token tok = p->current;
	struct ast_node *node;

	switch (tok.type) {
	case TK_HEX:
		return parse_numbers(p, 16, 2);
	case TK_OCTAL:
		return parse_numbers(p, 8, 2);
	case TK_BINARY:
		return parse_numbers(p, 2, 2);
	case TK_NUMBER:
		return parse_numbers(p, 10, 0);
	case TK_FLOATING:
		advance(p);
		node = ast_node_new(&p->arena, NODE_FLOATING, tok);
		node->lit_floating.val = xstrtod(tok.lex, tok.len);
		return node;
	case TK_CHARLIT:
		advance(p);
		node = ast_node_new(&p->arena, NODE_CHAR, tok);
		node->lit_char.val = tok.lex + 1;
		return node;
	case TK_STRINGLIT:
		advance(p);
		node = ast_node_new(&p->arena, NODE_STRING, tok);
		node->lit_str.val = tok.lex + 1;
		node->lit_str.len = tok.len - 2;
		return node;
	case TK_SIZE_OF:
		advance(p);
		expect(p, TK_LPAREN);
		node = ast_node_new(&p->arena, NODE_SIZEOF, tok);
		node->sizeof_expr.expr = parse_type(p);
		expect(p, TK_RPAREN);
		return node;
	case TK_SYSCALL: {
		size_t nr = 0, alloc = 0;
		advance(p);
		expect(p, TK_LPAREN);
		node = ast_node_new(&p->arena, NODE_SYSCALL, tok);
		while (!check(p, TK_RPAREN)) {
			ARENA_ALLOC_GROW(&p->arena, node->list.items, nr + 1, alloc);
			node->list.items[nr++] = parse_expr(p);
			if (!check(p, TK_RPAREN))
				expect(p, TK_COMMA);
		}
		expect(p, TK_RPAREN);
		node->list.nr_item = nr;
		return node;
	}
	case TK_LBRACKET: {
		size_t nr = 0, alloc = 0;
		advance(p);
		node = ast_node_new(&p->arena, NODE_ARRAY_INIT, tok);
		while (!check(p, TK_RBRACKET)) {
			ARENA_ALLOC_GROW(&p->arena, node->list.items, nr + 1, alloc);
			node->list.items[nr++] = parse_expr(p);
			if (!check(p, TK_RBRACKET))
				expect(p, TK_COMMA);
		}
		expect(p, TK_RBRACKET);
		node->list.nr_item = nr;
		return node;
	}
	case TK_TRUE:
	case TK_FALSE:
		advance(p);
		node = ast_node_new(&p->arena, NODE_BOOL, tok);
		node->lit_bool.val = tok.type == TK_TRUE;
		return node;
	case TK_NULL:
		advance(p);
		return ast_node_new(&p->arena, NODE_NULL, tok);
	case TK_ID:
		advance(p);
		if (!p->no_struct_init && check(p, TK_LBRACE)) {
			size_t nr = 0, alloc = 0;
			advance(p);
			node = ast_node_new(&p->arena, NODE_STRUCT_INIT, tok);
			while (!check(p, TK_RBRACE)) {
				ARENA_ALLOC_GROW(&p->arena, node->list.items, nr + 1, alloc);
				node->list.items[nr++] = parse_field_init(p);
				if (!check(p, TK_RBRACE))
					expect(p, TK_COMMA);
			}
			expect(p, TK_RBRACE);
			node->list.nr_item = nr;
			return node;
		}
		return ast_node_new(&p->arena, NODE_ID, tok);
	case TK_LPAREN:
		advance(p);
		node = parse_expr(p);
		expect(p, TK_RPAREN);
		return node;
	default:
		die("unexpected token at %d:%d", tok.line, tok.col);
	}
}

/*
 * #NEEDSWORK: Pratt parsing would fit nice to avoid having too many functions
 * that do the same thing
 */
static struct ast_node *parse_postfix(struct parser_context *p)
{
	struct ast_node *node = parse_primary(p);

	for (;;) {
		if (check(p, TK_DOT)) {
			advance(p);
			expect(p, TK_ID);
			struct ast_node *m = ast_node_new(&p->arena,
							  NODE_MEMBER,
							  p->prev);
			m->member.left = node;
			node = m;
		} else if (check(p, TK_NAMESPACE)) {
			advance(p);
			expect(p, TK_ID);
			struct ast_node *ns = ast_node_new(&p->arena,
							   NODE_NAMESPACE,
							   p->prev);
			ns->namespace.left = node;
			node = ns;
		} else if (check(p, TK_LPAREN)) {
			struct token tok = advance(p);
			struct ast_node *call = ast_node_new(&p->arena,
							     NODE_CALL, tok);
			size_t nr = 0, alloc = 0;

			call->call.callee = node;
			while (!check(p, TK_RPAREN)) {
				ARENA_ALLOC_GROW(&p->arena, call->call.args, nr + 1, alloc);
				call->call.args[nr++] = parse_expr(p);
				if (!check(p, TK_RPAREN))
					expect(p, TK_COMMA);
			}
			expect(p, TK_RPAREN);
			call->call.nr_arg = nr;
			node = call;
		} else if (check(p, TK_LBRACKET)) {
			struct token tok = advance(p);
			struct ast_node *idx = ast_node_new(&p->arena,
							    NODE_INDEX, tok);
			idx->index.obj = node;
			idx->index.idx = parse_expr(p);

			if (match(p, TK_COLON))
				idx->index.end = parse_expr(p);

			expect(p, TK_RBRACKET);
			node = idx;
		} else {
			break;
		}
	}
	return node;
}

static struct ast_node *parse_assign(struct parser_context *p)
{
	struct ast_node *left = parse_range(p);

	if (check(p, TK_EQUAL) || check(p, TK_PLUSEQ) ||
	    check(p, TK_MINUSEQ) || check(p, TK_STAREQ) ||
	    check(p, TK_SLASHEQ) || check(p, TK_MODEQ)) {
		struct token op = advance(p);
		struct ast_node *node = ast_node_new(&p->arena, NODE_ASSIGN, op);
		node->assign.type = token_to_op(op.type);
		node->assign.target = left;
		node->assign.val = parse_assign(p);
		return node;
	}

	return left;
}

static struct ast_node *parse_expr(struct parser_context *p)
{
	return parse_assign(p);
}

static struct ast_node *parse_unary(struct parser_context *p)
{
	if (check(p, TK_MINUS) || check(p, TK_NOT) ||
	    check(p, TK_TILDE) || check(p, TK_AMP) ||
	    check(p, TK_STAR)) {
		struct token op = advance(p);
		struct ast_node *node = ast_node_new(&p->arena, NODE_UNARY, op);
		node->unary.type = token_to_op(op.type);
		node->unary.operand = parse_unary(p);
		return node;
	}

	return parse_postfix(p);
}

static struct ast_node *parse_cast(struct parser_context *p)
{
	struct ast_node *left = parse_unary(p);

	while (check(p, TK_AS)) {
		struct token op = advance(p);
		struct ast_node *node = ast_node_new(&p->arena, NODE_CAST, op);
		node->cast.expr = left;
		node->cast.target_type = parse_type(p);
		left = node;
	}
	return left;
}

static struct ast_node *parse_mul(struct parser_context *p)
{
	struct ast_node *left = parse_cast(p);

	while (check(p, TK_STAR) || check(p, TK_SLASH) || check(p, TK_MOD)) {
		struct token op = advance(p);
		struct ast_node *node = ast_node_new(&p->arena, NODE_BINARY, op);
		node->binary.type = token_to_op(op.type);
		node->binary.left = left;
		node->binary.right = parse_cast(p);
		left = node;
	}
	return left;
}

static struct ast_node *parse_add(struct parser_context *p)
{
	struct ast_node *left = parse_mul(p);

	while (check(p, TK_PLUS) || check(p, TK_MINUS)) {
		struct token op = advance(p);
		struct ast_node *node = ast_node_new(&p->arena, NODE_BINARY, op);
		node->binary.type = token_to_op(op.type);
		node->binary.left = left;
		node->binary.right = parse_mul(p);
		left = node;
	}
	return left;
}

static struct ast_node *parse_shift(struct parser_context *p)
{
	struct ast_node *left = parse_add(p);

	while (check(p, TK_LSHIFT) || check(p, TK_RSHIFT)) {
		struct token op = advance(p);
		struct ast_node *node = ast_node_new(&p->arena, NODE_BINARY, op);
		node->binary.type = token_to_op(op.type);
		node->binary.left = left;
		node->binary.right = parse_add(p);
		left = node;
	}
	return left;
}

static struct ast_node *parse_cmp(struct parser_context *p)
{
	struct ast_node *left = parse_shift(p);

	while (check(p, TK_LT) || check(p, TK_GT) || check(p, TK_LE) || check(p, TK_GE)) {
		struct token op = advance(p);
		struct ast_node *node = ast_node_new(&p->arena, NODE_BINARY, op);
		node->binary.type = token_to_op(op.type);
		node->binary.left = left;
		node->binary.right = parse_shift(p);
		left = node;
	}
	return left;
}

static struct ast_node *parse_eq(struct parser_context *p)
{
	struct ast_node *left = parse_cmp(p);

	while (check(p, TK_CMP) || check(p, TK_NEQ)) {
		struct token op = advance(p);
		struct ast_node *node = ast_node_new(&p->arena, NODE_BINARY, op);
		node->binary.type = token_to_op(op.type);
		node->binary.left = left;
		node->binary.right = parse_cmp(p);
		left = node;
	}
	return left;
}

static struct ast_node *parse_bitand(struct parser_context *p)
{
	struct ast_node *left = parse_eq(p);

	while (check(p, TK_AMP)) {
		struct token op = advance(p);
		struct ast_node *node = ast_node_new(&p->arena, NODE_BINARY, op);
		node->binary.type = token_to_op(op.type);
		node->binary.left = left;
		node->binary.right = parse_eq(p);
		left = node;
	}
	return left;
}

static struct ast_node *parse_bitxor(struct parser_context *p)
{
	struct ast_node *left = parse_bitand(p);

	while (check(p, TK_CARET)) {
		struct token op = advance(p);
		struct ast_node *node = ast_node_new(&p->arena, NODE_BINARY, op);
		node->binary.type = token_to_op(op.type);
		node->binary.left = left;
		node->binary.right = parse_bitand(p);
		left = node;
	}
	return left;
}

static struct ast_node *parse_bitor(struct parser_context *p)
{
	struct ast_node *left = parse_bitxor(p);

	while (check(p, TK_PIPE)) {
		struct token op = advance(p);
		struct ast_node *node = ast_node_new(&p->arena, NODE_BINARY, op);
		node->binary.type = token_to_op(op.type);
		node->binary.left = left;
		node->binary.right = parse_bitxor(p);
		left = node;
	}
	return left;
}

static struct ast_node *parse_and(struct parser_context *p)
{
	struct ast_node *left = parse_bitor(p);

	while (check(p, TK_AND)) {
		struct token op = advance(p);
		struct ast_node *node = ast_node_new(&p->arena, NODE_BINARY, op);
		node->binary.type = token_to_op(op.type);
		node->binary.left = left;
		node->binary.right = parse_bitor(p);
		left = node;
	}
	return left;
}

static struct ast_node *parse_or(struct parser_context *p)
{
	struct ast_node *left = parse_and(p);

	while (check(p, TK_OR)) {
		struct token op = advance(p);
		struct ast_node *node = ast_node_new(&p->arena, NODE_BINARY, op);
		node->binary.type = token_to_op(op.type);
		node->binary.left = left;
		node->binary.right = parse_and(p);
		left = node;
	}
	return left;
}

static struct ast_node *parse_range(struct parser_context *p)
{
	struct ast_node *left = parse_or(p);

	if (check(p, TK_RANGE)) {
		struct token op = advance(p);
		struct ast_node *node = ast_node_new(&p->arena, NODE_BINARY, op);
		node->binary.type = token_to_op(op.type);
		node->binary.left = left;
		node->binary.right = parse_or(p);
		return node;
	}
	return left;
}

/*
 * STATEMENTS
 */

static inline struct ast_node *parse_expr_no_struct(struct parser_context *p)
{
	struct ast_node *node;

	p->no_struct_init = 1;
	node = parse_expr(p);
	p->no_struct_init = 0;
	return node;
}

static struct ast_node *parse_incdec(struct parser_context *p, struct ast_node *expr)
{
	struct token op = advance(p);
	struct ast_node *node = ast_node_new(&p->arena, NODE_INCDEC, op);
	node->incdec.type = token_to_op(op.type);
	node->incdec.target = expr;
	expect(p, TK_SEMICOLON);
	return node;
}

static struct ast_node *parse_expr_stmt(struct parser_context *p)
{
	struct ast_node *expr = parse_expr(p);

	if (check(p, TK_INCREMENT) || check(p, TK_DECREMENT))
		return parse_incdec(p, expr);

	struct ast_node *node = ast_node_new(&p->arena, NODE_EXPR_STMT, p->current);
	node->expr_stmt.expr = expr;
	expect(p, TK_SEMICOLON);
	return node;
}

static struct ast_node *parse_match_pattern(struct parser_context *p)
{
	struct token tok = p->current;
	struct ast_node *node;

	if (check(p, TK_UNDERSCORE)) {
		advance(p);
		node = ast_node_new(&p->arena, NODE_MATCH_PATTERN, tok);
		node->match_pattern.is_wildcard = 1;
		return node;
	}

	/*
	 * Is id tuple constructor
	 */
	if (check(p, TK_ID)) {
		struct token id = advance(p);
		node = ast_node_new(&p->arena, NODE_MATCH_PATTERN, id);
		node->match_pattern.expr = ast_node_new(&p->arena, NODE_ID, id);

		if (match(p, TK_LPAREN)) {
			node->match_pattern.bind = parse_id_list(p, &node->match_pattern.nr_bind);
			expect(p, TK_RPAREN);
		}
		return node;
	}

	/*
	 * Is a literal
	 */
	node = ast_node_new(&p->arena, NODE_MATCH_PATTERN, tok);
	node->match_pattern.expr = parse_expr(p);
	return node;
}

static struct ast_node *parse_match_arm(struct parser_context *p)
{
	struct token tok = p->current;
	struct ast_node *node = ast_node_new(&p->arena, NODE_MATCH_ARM, tok);
	node->match_arm.pattern = parse_match_pattern(p);
	expect(p, TK_FATARROW);
	node->match_arm.body = parse_stmt(p);
	return node;
}

static struct ast_node *parse_match(struct parser_context *p)
{
	struct token tok = advance(p);
	struct ast_node *node = ast_node_new(&p->arena, NODE_MATCH, tok);
	size_t nr = 0, alloc = 0;

	expect(p, TK_LPAREN);
	node->match.subj = parse_expr(p);
	expect(p, TK_RPAREN);
	expect(p, TK_LBRACE);
	while (!check(p, TK_RBRACE) && !check(p, TK_EOF)) {
		ARENA_ALLOC_GROW(&p->arena, node->match.arms, nr + 1, alloc);
		node->match.arms[nr++] = parse_match_arm(p);
	}
	expect(p, TK_RBRACE);
	node->match.nr_arm = nr;
	return node;
}

static struct ast_node *parse_defer(struct parser_context *p)
{
	struct token tok = advance(p);
	struct ast_node *node = ast_node_new(&p->arena, NODE_DEFER, tok);
	node->defer_stmt.stmt = parse_stmt(p);
	return node;
}

static struct ast_node *parse_continue(struct parser_context *p)
{
	struct token tok = advance(p);
	struct ast_node *node = ast_node_new(&p->arena, NODE_CONTINUE, tok);
	expect(p, TK_SEMICOLON);
	return node;
}

static struct ast_node *parse_break(struct parser_context *p)
{
	struct token tok = advance(p);
	struct ast_node *node = ast_node_new(&p->arena, NODE_BREAK, tok);
	expect(p, TK_SEMICOLON);
	return node;
}

static struct ast_node *parse_return(struct parser_context *p)
{
	struct token tok = advance(p);
	struct ast_node *node = ast_node_new(&p->arena, NODE_RETURN, tok);

	if (!check(p, TK_SEMICOLON))
		node->return_stmt.expr = parse_expr(p);

	expect(p, TK_SEMICOLON);
	return node;
}

static struct ast_node *parse_for(struct parser_context *p)
{
	struct ast_node *node;

	advance(p);
	expect(p, TK_ID);
	node = ast_node_new(&p->arena, NODE_FOR, p->prev);
	expect(p, TK_IN);
	node->for_stmt.range = parse_expr_no_struct(p);
	node->for_stmt.body = parse_stmt(p);
	return node;
}

static struct ast_node *parse_while(struct parser_context *p)
{
	struct ast_node *node;

	advance(p);
	node = ast_node_new(&p->arena, NODE_WHILE, p->prev);
	node->while_stmt.cond = parse_expr_no_struct(p);
	node->while_stmt.body = parse_stmt(p);
	return node;
}

static struct ast_node *parse_if(struct parser_context *p)
{
	struct ast_node *node;
	size_t nr = 0, alloc_bodies = 0, alloc_conds = 0;

	advance(p);
	node = ast_node_new(&p->arena, NODE_IF, p->prev);
	do {
		ARENA_ALLOC_GROW(&p->arena, node->if_stmt.conds, nr + 1,
				 alloc_conds);

		ARENA_ALLOC_GROW(&p->arena, node->if_stmt.bodies, nr + 1,
				 alloc_bodies);

		node->if_stmt.conds[nr] = parse_expr_no_struct(p);
		node->if_stmt.bodies[nr] = parse_stmt(p);
		nr++;
	} while (match(p, TK_ELIF));

	if (match(p, TK_ELSE))
		node->if_stmt.else_body = parse_stmt(p);

	node->if_stmt.nr_branch = nr;
	return node;
}

static struct ast_node *parse_block(struct parser_context *p)
{
	struct ast_node *node = ast_node_new(&p->arena, NODE_BLOCK, p->current);

	expect(p, TK_LBRACE);
	while (!check(p, TK_RBRACE) && !check(p, TK_EOF))
		ast_node_append(&p->arena, node, parse_stmt(p));
	expect(p, TK_RBRACE);

	return node;
}

static struct ast_node *parse_stmt(struct parser_context *p)
{
	switch (p->current.type) {
	case TK_LBRACE:
		return parse_block(p);
	case TK_IF:
		return parse_if(p);
	case TK_LOOP:
		return parse_while(p);
	case TK_FOR:
		return parse_for(p);
	case TK_RETURN:
		return parse_return(p);
	case TK_BREAK:
		return parse_break(p);
	case TK_CONTINUE:
		return parse_continue(p);
	case TK_DEFER:
		return parse_defer(p);
	case TK_MATCH:
		return parse_match(p);
	default:
		return parse_expr_stmt(p);
	}
}

/*
 * DECLARATIONS
 */

static struct token *parse_id_list(struct parser_context *p, size_t *nr)
{
	struct token *list = NULL;
	size_t c = 0, alloc = 0;

	do {
		expect(p, TK_ID);
		ARENA_ALLOC_GROW(&p->arena, list, c + 1, alloc);
		list[c++] = p->prev;
	} while (match(p, TK_COMMA));

	*nr = c;
	return list;
}

static struct ast_node *parse_function(struct parser_context *p)
{
	struct ast_node *node;
	size_t nr = 0, alloc = 0;

	expect(p, TK_FN);
	expect(p, TK_ID);
	node = ast_node_new(&p->arena, NODE_FN_DEC, p->prev);

	expect(p, TK_LPAREN);
	while (!check(p, TK_RPAREN)) {
		if (match(p, TK_SPREAD)) {
			node->fn_dec.is_variadic = 1;
			break;
		}
		expect(p, TK_ID);
		struct ast_node *param = ast_node_new(&p->arena, NODE_PARAM,
						      p->prev);
		expect(p, TK_COLON);
		param->typed.ann = parse_type(p);

		ARENA_ALLOC_GROW(&p->arena, node->fn_dec.params, nr + 1, alloc);
		node->fn_dec.params[nr++] = param;
		if (!check(p, TK_RPAREN))
			expect(p, TK_COMMA);
	}
	expect(p, TK_RPAREN);
	node->fn_dec.nr_param = nr;

	if (match(p, TK_RARROW))
		node->fn_dec.ret_type = parse_type(p);

	node->fn_dec.body = parse_stmt(p);
	return node;
}

static struct ast_node *parse_struct(struct parser_context *p)
{
	struct ast_node *node;
	size_t nr = 0, alloc = 0;

	expect(p, TK_STRUCT);
	expect(p, TK_ID);
	node = ast_node_new(&p->arena, NODE_STRUCT_DEC, p->prev);

	expect(p, TK_LBRACE);
	while (!check(p, TK_RBRACE) && !check(p, TK_EOF)) {
		expect(p, TK_ID);
		struct ast_node *field = ast_node_new(&p->arena, NODE_FIELD, p->prev);

		expect(p, TK_COLON);
		field->typed.ann = parse_type(p);

		ARENA_ALLOC_GROW(&p->arena, node->list.items, nr + 1, alloc);
		node->list.items[nr++] = field;
		if (!check(p, TK_RBRACE))
			expect(p, TK_SEMICOLON);
	}
	expect(p, TK_RBRACE);
	node->list.nr_item = nr;
	return node;
}

static struct ast_node *parse_impl(struct parser_context *p)
{
	struct ast_node *node;
	size_t nr = 0, alloc = 0;

	expect(p, TK_IMPL);
	expect(p, TK_ID);
	node = ast_node_new(&p->arena, NODE_IMPL_DEC, p->prev);
	expect(p, TK_LBRACE);
	while (!check(p, TK_RBRACE) && !check(p, TK_EOF)) {
		struct ast_node *fn = parse_function(p);
		ARENA_ALLOC_GROW(&p->arena, node->list.items, nr + 1, alloc);
		node->list.items[nr++] = fn;
	}
	expect(p, TK_RBRACE);
	node->list.nr_item = nr;
	return node;
}

static struct ast_node *parse_enum(struct parser_context *p)
{
	struct ast_node *node;
	size_t nr = 0, alloc = 0;

	expect(p, TK_ENUM);
	expect(p, TK_ID);
	node = ast_node_new(&p->arena, NODE_ENUM_DEC, p->prev);
	expect(p, TK_LBRACE);
	while (!check(p, TK_RBRACE) && !check(p, TK_EOF)) {
		size_t nr_assocs = 0, alloc_assocs = 0;

		expect(p, TK_ID);
		struct ast_node *member = ast_node_new(&p->arena, NODE_ENUM_MEMBER, p->prev);

		if (match(p, TK_LPAREN)) {
			while (!check(p, TK_RPAREN) && !check(p, TK_EOF)) {
				ARENA_ALLOC_GROW(&p->arena,
						 member->enum_member.assocs,
						 nr_assocs + 1,
						 alloc_assocs);

				member->enum_member.assocs[nr_assocs++] = parse_type(p);

				if (!check(p, TK_RPAREN))
					expect(p, TK_COMMA);
			}
			expect(p, TK_RPAREN);
			member->enum_member.nr_assoc = nr_assocs;
		}

		if (match(p, TK_EQUAL))
			member->enum_member.val = parse_expr(p);

		ARENA_ALLOC_GROW(&p->arena, node->list.items, nr + 1, alloc);
		node->list.items[nr++] = member;

		if (!check(p, TK_RBRACE))
			expect(p, TK_COMMA);
	}
	expect(p, TK_RBRACE);
	node->list.nr_item = nr;
	return node;
}

static struct ast_node *parse_type_dec(struct parser_context *p)
{
	struct ast_node *node;

	expect(p, TK_TYPE);
	expect(p, TK_ID);
	node = ast_node_new(&p->arena, NODE_TYPE_DEC, p->prev);

	expect(p, TK_EQUAL);
	node->type_dec.type = parse_type(p);
	expect(p, TK_SEMICOLON);
	return node;
}

/*
 * There can be 3 types of let declarations:
 *
 *   1. let foo: type = init;
 *   2. let foo: type;
 *   3. let foo = init; (here type will adopt the type from init)
 *
 * Therefore, there cannot be a 'let foo;' declaration due to not being something
 * to get the type from nor explicitly a type to relate to.
 */
static struct ast_node *parse_let(struct parser_context *p)
{
	struct ast_node *node;

	expect(p, TK_LET);
	node = ast_node_new(&p->arena, NODE_LET_DEC, p->prev);

	if (match(p, TK_LPAREN)) {
		node->let_dec.name = parse_id_list(p, &node->let_dec.nr_name);
		expect(p, TK_RPAREN);
	} else {
		expect(p, TK_ID);
		node->let_dec.name = arena_alloc(&p->arena, sizeof(struct token));
		*node->let_dec.name = p->prev;
		node->let_dec.nr_name = 1;
	}

	if (match(p, TK_COLON)) {
		node->let_dec.ann = parse_type(p);
		if (match(p, TK_EQUAL))
			node->let_dec.init = parse_expr(p);
	} else if (match(p, TK_EQUAL)) {
		node->let_dec.init = parse_expr(p);
	} else {
		die("let declaration needs a type or a initializer at least at %d:%d",
		    p->current.line,
		    p->current.col);
	}

	expect(p, TK_SEMICOLON);
	return node;
}

static struct ast_node *parse_const(struct parser_context *p)
{
	struct ast_node *node;

	expect(p, TK_CONST);
	expect(p, TK_ID);
	node = ast_node_new(&p->arena, NODE_CONST_DEC, p->prev);
	expect(p, TK_COLON);
	node->const_dec.ann = parse_type(p);
	expect(p, TK_EQUAL);
	node->const_dec.init = parse_expr(p);
	expect(p, TK_SEMICOLON);
	return node;
}

static struct ast_node *parse_import(struct parser_context *p)
{
	struct ast_node *node;

	expect(p, TK_IMPORT);
	expect(p, TK_STRINGLIT);
	node = ast_node_new(&p->arena, NODE_IMPORT_DEC, p->prev);
	expect(p, TK_SEMICOLON);
	return node;
}

static struct ast_node *parse_dec(struct parser_context *p)
{
	switch (p->current.type) {
	case TK_FN:
		return parse_function(p);
	case TK_STRUCT:
		return parse_struct(p);
	case TK_IMPL:
		return parse_impl(p);
	case TK_ENUM:
		return parse_enum(p);
	case TK_TYPE:
		return parse_type_dec(p);
	case TK_LET:
		return parse_let(p);
	case TK_CONST:
		return parse_const(p);
	case TK_IMPORT:
		return parse_import(p);
	default:
		return parse_stmt(p);
	}
}

void parser_init(struct parser_context *p, struct lexer_context *lexer)
{
	memset(p, 0, sizeof(*p));
	p->lexer = lexer;
	arena_init(&p->arena, PARSER_ARENA_DEF);
	p->current = token_next(lexer);
}

/*
 * declarations -> statements | expressions
 *
 * An expression cannot live on its own, they live through statements or
 * declarations
 */
struct ast_node *parser_parse(struct parser_context *p)
{
	struct ast_node *program = ast_node_new(&p->arena, NODE_PROGRAM, p->current);

	while (!check(p, TK_EOF))
		ast_node_append(&p->arena, program, parse_dec(p));

	return program;
}

void parser_free(struct parser_context *p)
{
	arena_free(&p->arena);
}
