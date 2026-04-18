#include "parser/ast.h"
#include "memory/wrapper.h"
#include "memory/arena.h"

#include <stdio.h>
#include <string.h>

struct ast_node *ast_node_new(struct arena *a, enum node_type type,
			      struct token tok)
{
	struct ast_node *node = arena_alloc(a, sizeof(*node));
	memset(node, 0, sizeof(*node));
	node->type = type;
	node->tok = tok;
	return node;
}

void ast_node_append(struct arena *a, struct ast_node *node,
		     struct ast_node *child)
{
	ARENA_ALLOC_GROW(a, node->block.childs, node->block.nr + 1,
			 node->block.alloc);
	node->block.childs[node->block.nr++] = child;
}

enum op_type token_to_op(enum token_type type)
{
	switch (type) {
	case TK_PLUS:
		return OP_ADD;
	case TK_MINUS:
		return OP_SUB;
	case TK_STAR:
		return OP_MUL;
	case TK_SLASH:
		return OP_DIV;
	case TK_MOD:
		return OP_MOD;
	case TK_CMP:
		return OP_EQ;
	case TK_NEQ:
		return OP_NEQ;
	case TK_LT:
		return OP_LT;
	case TK_GT:
		return OP_GT;
	case TK_LE:
		return OP_LE;
	case TK_GE:
		return OP_GE;
	case TK_AND:
		return OP_AND;
	case TK_OR:
		return OP_OR;
	case TK_AMP:
		return OP_BIT_AND;
	case TK_PIPE:
		return OP_BIT_OR;
	case TK_CARET:
		return OP_BIT_XOR;
	case TK_LSHIFT:
		return OP_LSHIFT;
	case TK_RSHIFT:
		return OP_RSHIFT;
	case TK_RANGE:
		return OP_RANGE;
	case TK_NOT:
		return OP_NOT;
	case TK_TILDE:
		return OP_BIT_NOT;
	case TK_EQUAL:
		return OP_ASSIGN;
	case TK_PLUSEQ:
		return OP_PLUSEQ;
	case TK_MINUSEQ:
		return OP_MINUSEQ;
	case TK_STAREQ:
		return OP_STAREQ;
	case TK_SLASHEQ:
		return OP_SLASHEQ;
	case TK_MODEQ:
		return OP_MODEQ;
	case TK_INCREMENT:
		return OP_INC;
	case TK_DECREMENT:
		return OP_DEC;
	default:
		die("unknown operator token %d", type);
	}
}

const char *op_name(enum op_type op)
{
	static const char *names[] = {
		[OP_ADD] = "+",
		[OP_SUB] = "-",
		[OP_MUL] = "*",
		[OP_DIV] = "/",
		[OP_MOD] = "%",
		[OP_EQ] = "==",
		[OP_NEQ] = "!=",
		[OP_LT] = "<",
		[OP_GT] = ">",
		[OP_LE] = "<=",
		[OP_GE] = ">=",
		[OP_AND] = "&&",
		[OP_OR] = "||",
		[OP_BIT_AND] = "&",
		[OP_BIT_OR] = "|",
		[OP_BIT_XOR] = "^",
		[OP_LSHIFT] = "<<",
		[OP_RSHIFT] = ">>",
		[OP_RANGE] = "..",
		[OP_NEG] = "-",
		[OP_NOT] = "!",
		[OP_BIT_NOT] = "~",
		[OP_ADDR] = "&",
		[OP_DEREF] = "*",
		[OP_ASSIGN] = "=",
		[OP_PLUSEQ] = "+=",
		[OP_MINUSEQ] = "-=",
		[OP_STAREQ] = "*=",
		[OP_SLASHEQ] = "/=",
		[OP_MODEQ] = "%=",
		[OP_INC] = "++",
		[OP_DEC] = "--",
	};

	if ((size_t)op < ARRAY_SIZE(names) && names[op])
		return names[op];
	return "unknown";
}

static void do_ast_dump(struct ast_node *node, int depth, int last, int *prefix)
{
	int i;

	if (!node)
		return;

	for (i = 1; i < depth; i++)
		printf("%s", prefix[i] ? "    " : "|   ");
	if (depth > 0)
		printf("%s", last ? "`-- " : "|-- ");

	prefix[depth] = last;

	switch (node->type) {
	case NODE_PROGRAM:
	case NODE_BLOCK:
		fprintf(stdout, "%s\n", node->type == NODE_PROGRAM ? "PROGRAM" : "BLOCK");
		for (i = 0; i < (int)node->block.nr; i++)
			do_ast_dump(node->block.childs[i], depth + 1,
				    i == (int)node->block.nr - 1, prefix);
		break;
	case NODE_EXPR_STMT:
		fprintf(stdout, "EXPR_STMT\n");
		do_ast_dump(node->expr_stmt.expr, depth + 1, 1, prefix);
		break;
	case NODE_INT:
		printf("INT %lld\n", node->lit_int.val);
		break;
	case NODE_FLOATING:
		printf("FLOAT %f\n", node->lit_floating.val);
		break;
	case NODE_STRING:
		printf("STRING '%.*s'\n",
		       (int)node->lit_str.len, node->lit_str.val);
		break;
	case NODE_CHAR:
		printf("CHAR '%s'\n", node->lit_char.val);
		break;
	case NODE_BOOL:
		printf("BOOL %s\n",
		       node->lit_bool.val ? "true" : "false");
		break;
	case NODE_NULL:
		printf("NULL\n");
		break;
	case NODE_ID:
		printf("ID '%.*s'\n",
		       (int)node->tok.len, node->tok.lex);
		break;
	case NODE_BINARY:
		printf("BINARY (%s)\n", op_name(node->binary.type));
		do_ast_dump(node->binary.left, depth + 1, 0, prefix);
		do_ast_dump(node->binary.right, depth + 1, 1, prefix);
		break;
	case NODE_UNARY:
		printf("UNARY (%s)\n", op_name(node->unary.type));
		do_ast_dump(node->unary.operand, depth + 1, 1, prefix);
		break;
	case NODE_ASSIGN:
		printf("ASSIGN (%s)\n", op_name(node->assign.type));
		do_ast_dump(node->assign.target, depth + 1, 0, prefix);
		do_ast_dump(node->assign.val, depth + 1, 1, prefix);
		break;
	case NODE_CALL:
		printf("CALL\n");
		do_ast_dump(node->call.callee, depth + 1,
			    node->call.nr_arg == 0, prefix);
		for (i = 0; i < (int)node->call.nr_arg; i++)
			do_ast_dump(node->call.args[i], depth + 1,
				    i == (int)node->call.nr_arg - 1, prefix);
		break;
	case NODE_INDEX:
		printf("INDEX\n");
		do_ast_dump(node->index.obj, depth + 1, 0, prefix);
		if (node->index.end) {
			do_ast_dump(node->index.idx, depth + 1, 0, prefix);
			do_ast_dump(node->index.end, depth + 1, 1, prefix);
		} else {
			do_ast_dump(node->index.idx, depth + 1, 1, prefix);
		}
		break;
	case NODE_MEMBER:
		printf("MEMBER '%.*s'\n",
		       (int)node->tok.len, node->tok.lex);
		do_ast_dump(node->member.left, depth + 1, 1, prefix);
		break;
	case NODE_NAMESPACE:
		printf("NAMESPACE '%.*s'\n",
		       (int)node->tok.len, node->tok.lex);
		do_ast_dump(node->namespace.left, depth + 1, 1, prefix);
		break;
	case NODE_CAST:
		printf("CAST\n");
		do_ast_dump(node->cast.expr, depth + 1, 0, prefix);
		do_ast_dump(node->cast.target_type, depth + 1, 1, prefix);
		break;
	case NODE_SIZEOF:
		printf("SIZEOF\n");
		do_ast_dump(node->sizeof_expr.expr, depth + 1, 1, prefix);
		break;
	case NODE_STRUCT_INIT:
	case NODE_ARRAY_INIT:
	case NODE_SYSCALL:
	case NODE_TYPE_TUPLE:
		printf("%s (%zu items)\n",
		       node->type == NODE_STRUCT_INIT ? "STRUCT_INIT" :
		       node->type == NODE_ARRAY_INIT  ? "ARRAY_INIT" :
		       node->type == NODE_SYSCALL     ? "SYSCALL" :
							"TYPE_TUPLE",
		       node->list.nr_item);
		for (i = 0; i < (int)node->list.nr_item; i++)
			do_ast_dump(node->list.items[i], depth + 1,
				    i == (int)node->list.nr_item - 1,
				    prefix);
		break;
	case NODE_FIELD_INIT:
		printf("FIELD_INIT '%.*s'\n",
		       (int)node->tok.len, node->tok.lex);
		do_ast_dump(node->field_init.val, depth + 1, 1, prefix);
		break;
	case NODE_TYPE_NAME:
		printf("TYPE '%.*s'\n",
		       (int)node->tok.len, node->tok.lex);
		break;
	case NODE_TYPE_PTR:
		printf("TYPE_PTR\n");
		do_ast_dump(node->type_ptr.type, depth + 1, 1, prefix);
		break;
	case NODE_TYPE_ARR:
		printf("TYPE_ARR [%lld]\n", node->type_array.size);
		do_ast_dump(node->type_array.elem_type, depth + 1, 1,
			    prefix);
		break;
	case NODE_IF:
		fprintf(stdout, "IF (%zu branch%s)\n",
			node->if_stmt.nr_branch,
			node->if_stmt.nr_branch > 1 ? "es" : "");
		for (i = 0; i < (int)node->if_stmt.nr_branch; i++) {
			do_ast_dump(node->if_stmt.conds[i], depth + 1, 0, prefix);
			do_ast_dump(node->if_stmt.bodies[i], depth + 1,
				    !node->if_stmt.else_body &&
					    i == (int)node->if_stmt.nr_branch - 1,
				    prefix);
		}
		if (node->if_stmt.else_body)
			do_ast_dump(node->if_stmt.else_body, depth + 1, 1, prefix);
		break;
	case NODE_WHILE:
		fprintf(stdout, "WHILE\n");
		do_ast_dump(node->while_stmt.cond, depth + 1, 0, prefix);
		do_ast_dump(node->while_stmt.body, depth + 1, 1, prefix);
		break;
	case NODE_FOR:
		fprintf(stdout, "FOR '%.*s'\n",
			(int)node->tok.len, node->tok.lex);
		do_ast_dump(node->for_stmt.range, depth + 1, 0, prefix);
		do_ast_dump(node->for_stmt.body, depth + 1, 1, prefix);
		break;
	case NODE_RETURN:
		fprintf(stdout, "RETURN\n");
		if (node->return_stmt.expr)
			do_ast_dump(node->return_stmt.expr, depth + 1, 1, prefix);
		break;
	case NODE_BREAK:
		fprintf(stdout, "BREAK\n");
		break;
	case NODE_CONTINUE:
		fprintf(stdout, "CONTINUE\n");
		break;
	case NODE_DEFER:
		fprintf(stdout, "DEFER\n");
		do_ast_dump(node->defer_stmt.stmt, depth + 1, 1, prefix);
		break;
	case NODE_INCDEC:
		fprintf(stdout, "INCDEC (%s)\n", op_name(node->incdec.type));
		do_ast_dump(node->incdec.target, depth + 1, 1, prefix);
		break;
	case NODE_MATCH:
		fprintf(stdout, "MATCH\n");
		do_ast_dump(node->match.subj, depth + 1,
			    node->match.nr_arm == 0, prefix);
		for (i = 0; i < (int)node->match.nr_arm; i++)
			do_ast_dump(node->match.arms[i], depth + 1,
				    i == (int)node->match.nr_arm - 1, prefix);
		break;
	case NODE_MATCH_ARM:
		fprintf(stdout, "MATCH_ARM\n");
		do_ast_dump(node->match_arm.pattern, depth + 1, 0, prefix);
		do_ast_dump(node->match_arm.body, depth + 1, 1, prefix);
		break;
	case NODE_MATCH_PATTERN:
		if (node->match_pattern.is_wildcard) {
			fprintf(stdout, "MATCH_PATTERN (wildcard)\n");
		} else if (node->match_pattern.nr_bind > 0) {
			fprintf(stdout, "MATCH_PATTERN (bind:");
			for (i = 0; i < (int)node->match_pattern.nr_bind; i++)
				fprintf(stdout, " %.*s",
					(int)node->match_pattern.bind[i].len,
					node->match_pattern.bind[i].lex);
			fprintf(stdout, ")\n");
			do_ast_dump(node->match_pattern.expr, depth + 1, 1, prefix);
		} else {
			fprintf(stdout, "MATCH_PATTERN\n");
			do_ast_dump(node->match_pattern.expr, depth + 1, 1, prefix);
		}
		break;
	case NODE_FN_DEC:
		fprintf(stdout, "FN_DEC '%.*s'\n",
			(int)node->tok.len, node->tok.lex);
		for (i = 0; i < (int)node->fn_dec.nr_param; i++)
			do_ast_dump(node->fn_dec.params[i], depth + 1, 0, prefix);
		if (node->fn_dec.ret_type)
			do_ast_dump(node->fn_dec.ret_type, depth + 1, 0, prefix);
		do_ast_dump(node->fn_dec.body, depth + 1, 1, prefix);
		break;
	case NODE_STRUCT_DEC:
	case NODE_IMPL_DEC:
	case NODE_ENUM_DEC:
		fprintf(stdout, "%s '%.*s'\n",
			node->type == NODE_STRUCT_DEC ? "STRUCT_DEC" :
			node->type == NODE_IMPL_DEC   ? "IMPL_DEC" :
							"ENUM_DEC",
			(int)node->tok.len, node->tok.lex);
		for (i = 0; i < (int)node->list.nr_item; i++)
			do_ast_dump(node->list.items[i], depth + 1,
				    i == (int)node->list.nr_item - 1, prefix);
		break;
	case NODE_ENUM_MEMBER:
		fprintf(stdout, "ENUM_MEMBER '%.*s'\n",
			(int)node->tok.len, node->tok.lex);
		for (i = 0; i < (int)node->enum_member.nr_assoc; i++)
			do_ast_dump(node->enum_member.assocs[i], depth + 1,
				    !node->enum_member.val &&
					    i == (int)node->enum_member.nr_assoc - 1,
				    prefix);
		if (node->enum_member.val)
			do_ast_dump(node->enum_member.val, depth + 1, 1, prefix);
		break;
	case NODE_TYPE_DEC:
		fprintf(stdout, "TYPE_DEC '%.*s'\n",
			(int)node->tok.len, node->tok.lex);
		do_ast_dump(node->type_dec.type, depth + 1, 1, prefix);
		break;
	case NODE_LET_DEC:
		if (node->let_dec.nr_name == 1) {
			fprintf(stdout, "LET_DEC '%.*s'\n",
				(int)node->let_dec.name[0].len,
				node->let_dec.name[0].lex);
		} else {
			fprintf(stdout, "LET_DEC (");
			for (i = 0; i < (int)node->let_dec.nr_name; i++) {
				if (i > 0)
					fprintf(stdout, ", ");
				fprintf(stdout, "%.*s",
					(int)node->let_dec.name[i].len,
					node->let_dec.name[i].lex);
			}
			fprintf(stdout, ")\n");
		}
		if (node->let_dec.ann)
			do_ast_dump(node->let_dec.ann, depth + 1,
				    !node->let_dec.init, prefix);
		if (node->let_dec.init)
			do_ast_dump(node->let_dec.init, depth + 1, 1, prefix);
		break;
	case NODE_CONST_DEC:
		fprintf(stdout, "CONST_DEC '%.*s'\n",
			(int)node->tok.len, node->tok.lex);
		do_ast_dump(node->const_dec.ann, depth + 1, 0, prefix);
		do_ast_dump(node->const_dec.init, depth + 1, 1, prefix);
		break;
	case NODE_IMPORT_DEC:
		fprintf(stdout, "IMPORT '%.*s'\n",
			(int)node->tok.len - 2, node->tok.lex + 1);
		break;
	case NODE_PARAM:
	case NODE_FIELD:
		fprintf(stdout, "%s '%.*s'\n",
			node->type == NODE_PARAM ? "PARAM" : "FIELD",
			(int)node->tok.len, node->tok.lex);
		if (node->typed.ann)
			do_ast_dump(node->typed.ann, depth + 1, 1, prefix);
		break;
	case NODE_ERROR:
		fprintf(stdout, "ERROR\n");
		break;
	default:
		printf("UNKNOWN node_type=%d\n", node->type);
		break;
	}
}

void ast_dump(struct ast_node *node)
{
	/* max depth, 256 should be plenty */
	int prefix[256] = { 0 };
	do_ast_dump(node, 0, 1, prefix);
}
