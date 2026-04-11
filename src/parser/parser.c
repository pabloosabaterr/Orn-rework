#include "parser.h"
#include "lexer/lexer.h"
#include "memory/arena.h"
#include "memory/wrapper.h"
#include "parser/ast.h"

#include <string.h>

static struct ast_node *parse_expr(struct parser_context *p);
static struct ast_node *parse_or(struct parser_context *p);
static struct ast_node *parse_unary(struct parser_context *p);

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

static struct ast_node *parse_base_type(struct parser_context *p)
{
	struct token tok = advance(p);

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

	return parse_base_type(p);
}

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
		if (check(p, TK_LBRACE)) {
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
	struct ast_node *left = parse_or(p);

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

void parser_init(struct parser_context *p, struct lexer_context *lexer)
{
	memset(p, 0, sizeof(*p));
	p->lexer = lexer;
	arena_init(&p->arena, PARSER_ARENA_DEF);
	p->current = token_next(lexer);
}

struct ast_node *parser_parse_expr(struct parser_context *p)
{
	return parse_expr(p);
}

void parser_free(struct parser_context *p)
{
	arena_free(&p->arena);
}
