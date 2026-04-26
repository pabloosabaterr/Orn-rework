#include "ir/ir.h"
#include "compiler.h"
#include "memory/arena.h"
#include "parser/ast.h"
#include "semantic/semantic.h"
#include "memory/wrapper.h"
#include "semantic/type.h"
#include <assert.h>
#include <string.h>

static enum ir_op ast_op_to_ir(enum op_type op)
{
	switch (op) {
	case OP_ADD:
		return IR_ADD;
	case OP_SUB:
		return IR_SUB;
	case OP_MUL:
		return IR_MUL;
	case OP_DIV:
		return IR_DIV;
	case OP_MOD:
		return IR_MOD;
	case OP_BIT_AND:
		return IR_AND;
	case OP_BIT_OR:
		return IR_OR;
	case OP_BIT_XOR:
		return IR_XOR;
	case OP_LSHIFT:
		return IR_SHL;
	case OP_RSHIFT:
		return IR_SHR;
	case OP_EQ:
		return IR_EQ;
	case OP_NEQ:
		return IR_NEQ;
	case OP_LT:
		return IR_LT;
	case OP_GT:
		return IR_GT;
	case OP_LE:
		return IR_LE;
	case OP_GE:
		return IR_GE;
	default:
		die("unhandled op in ast_op_to_ir: %d", op);
	}
}

static int is_cmp_op(enum ir_op op)
{
	return op == IR_EQ || op == IR_NEQ || op == IR_LT || op == IR_GT ||
	       op == IR_LE || op == IR_GE;
}

static struct ir_type *prim_new(struct ir_context *ic, enum ir_kind kind)
{
	struct ir_type *t = arena_alloc(&ic->cc->arena, sizeof(*t));
	memset(t, 0, sizeof(*t));
	t->kind = kind;
	return t;
}

static struct ir_type *type_ptr(struct ir_context *ic, struct ir_type *pointee)
{
	size_t i;
	struct ir_type *t;

	for (i = 0; i < ic->nr_ptr; i++)
		if (ic->ptrs[i]->ptr.pointee == pointee)
			return ic->ptrs[i];

	t = arena_alloc(&ic->cc->arena, sizeof(*t));
	memset(t, 0, sizeof(*t));

	t->kind = IR_PTR;
	t->ptr.pointee = pointee;

	ARENA_ALLOC_GROW(&ic->cc->arena, ic->ptrs, ic->nr_ptr + 1, ic->alloc_ptr);
	ic->ptrs[ic->nr_ptr++] = t;
	return t;
}

static struct ir_type *type_arr(struct ir_context *ic,
				struct ir_type *elem_type, long long size)
{
	size_t i;
	struct ir_type *t;

	for (i = 0; i < ic->nr_arr; i++) {
		struct ir_type *arr = ic->arrs[i];
		if (arr->arr.elem == elem_type && arr->arr.size == size)
			return ic->arrs[i];
	}

	t = arena_alloc(&ic->cc->arena, sizeof(*t));
	memset(t, 0, sizeof(*t));

	t->kind = IR_ARR;
	t->arr.elem = elem_type;
	t->arr.size = size;

	ARENA_ALLOC_GROW(&ic->cc->arena, ic->arrs, ic->nr_arr + 1, ic->alloc_arr);
	ic->arrs[ic->nr_arr++] = t;
	return t;
}

static struct ir_type *type_obj(struct ir_context *ic, struct ir_type **fields,
				size_t nr_field)
{
	size_t i;
	struct ir_type *t;

	for (i = 0; i < ic->nr_obj; i++) {
		t = ic->objs[i];
		if (t->obj.nr_fields != nr_field)
			continue;
		if (!memcmp(t->obj.fields, fields, nr_field * sizeof(*fields)))
			return t;
	}

	t = arena_alloc(&ic->cc->arena, sizeof(*t));
	memset(t, 0, sizeof(*t));

	t->kind = IR_STRUCT;
	t->obj.fields = fields;
	t->obj.nr_fields = nr_field;

	ARENA_ALLOC_GROW(&ic->cc->arena, ic->objs, ic->nr_obj + 1, ic->alloc_obj);
	ic->objs[ic->nr_obj++] = t;
	return t;
}

static struct ir_type *type_fn(struct ir_context *ic, struct ir_type **params,
			       size_t nr_param, struct ir_type *ret)
{
	size_t i;
	struct ir_type *t;

	for (i = 0; i < ic->nr_fn; i++) {
		t = ic->fns[i];
		if (t->fn.nr_param != nr_param || t->fn.ret != ret)
			continue;
		if (!memcmp(t->fn.params, params, nr_param * sizeof(*params)))
			return t;
	}

	t = arena_alloc(&ic->cc->arena, sizeof(*t));
	memset(t, 0, sizeof(*t));

	t->kind = IR_FN;
	t->fn.params = params;
	t->fn.nr_param = nr_param;
	t->fn.ret = ret;

	ARENA_ALLOC_GROW(&ic->cc->arena, ic->fns, ic->nr_fn + 1, ic->alloc_fn);
	ic->fns[ic->nr_fn++] = t;
	return t;
}

static size_t ir_type_alignof(struct ir_type *t)
{
	switch (t->kind) {
	case IR_I1:
		return 1;
	case IR_I8:
		return 1;
	case IR_I32:
		return 4;
	case IR_I64:
		return 8;
	case IR_F32:
		return 4;
	case IR_F64:
		return 8;
	case IR_PTR:
		return 8;
	case IR_VOID:
		return 1;
	case IR_ARR:
		return ir_type_alignof(t->arr.elem);
	case IR_STRUCT: {
		size_t max_align = 1, i;
		for (i = 0; i < t->obj.nr_fields; i++) {
			size_t a = ir_type_alignof(t->obj.fields[i]);
			if (a > max_align)
				max_align = a;
		}
		return max_align;
	}
	case IR_FN:
		return 8;
	default:
		assert(0 && "unknown align for type at ir_type_alignof()");
		return 1;
	}
}

static size_t ir_type_sizeof(struct ir_type *t)
{
	switch (t->kind) {
	case IR_I1:
		return 1;
	case IR_I8:
		return 1;
	case IR_I32:
		return 4;
	case IR_I64:
		return 8;
	case IR_F32:
		return 4;
	case IR_F64:
		return 8;
	case IR_PTR:
		return 8;
	case IR_VOID:
		return 0;
	case IR_ARR:
		return ir_type_sizeof(t->arr.elem) * t->arr.size;
	case IR_STRUCT: {
		size_t offset = 0, i;
		for (i = 0; i < t->obj.nr_fields; i++) {
			size_t field_align = ir_type_alignof(t->obj.fields[i]);
			offset = align_up(offset, field_align);
			offset += ir_type_sizeof(t->obj.fields[i]);
		}
		offset = align_up(offset, ir_type_alignof(t));
		return offset;
	}
	default:
		die("unknown sizeof at ir_type_sizeof()");
	}
}

static struct ir_type *ir_sem_type_lowering(struct ir_context *ic, struct type *sem_type)
{
	switch (sem_type->kind) {
	case TY_BOOL:
		return ic->t_i1;
	case TY_CHAR:
		return ic->t_i8;
	case TY_FLOAT:
		return ic->t_f32;
	case TY_DOUBLE:
		return ic->t_f64;
	case TY_INT:
		return ic->t_i32;
	case TY_UINT:
		return ic->t_i64;
	case TY_VOID:
		return ic->t_void;
	case TY_NULL:
		return type_ptr(ic, ic->t_void);
	case TY_STRING:
		return type_ptr(ic, ic->t_i8);
	case TY_PTR:
		return type_ptr(ic, ir_sem_type_lowering(ic, sem_type->ptr.pointee));
	case TY_ARR:
		return type_arr(ic, ir_sem_type_lowering(ic, sem_type->arr.elem_type),
				sem_type->arr.size);
	case TY_STRUCT: {
		size_t i;
		struct ir_type **fields;
		struct symbol *agg = sem_type->named.dec;

		fields = arena_alloc(&ic->cc->arena,
				     sizeof(*fields) * agg->aggregate.nr);
		for (i = 0; i < agg->aggregate.nr; i++)
			fields[i] = ir_sem_type_lowering(ic, agg->aggregate.members[i]->type);

		return type_obj(ic, fields, agg->aggregate.nr);
	}
	/*
	 * An enum with payload si lowered to a struct with two fields that can
	 * hold every member of the given enum.:
	 *
	 * { i32, [T; N] payload }
	 *
	 * Tag identifies wich member is and the payload is a flat array large
	 * enough to hold the largest payload.
	 *
	 * T is chosen to preserve the alignment for the biggest alignment
	 * needed:
	 *
	 * max_align == 8 -> T = i64
	 * max_align == 4 -> T = i32
	 * m -> T = i8
	 *
	 * Enums without payload gets lowered to i32
	 */
	case TY_ENUM: {
		struct symbol *agg = sem_type->named.dec;
		size_t i, max_size = 0, max_align = 1, data_count;
		int has_data = 0;
		struct ir_type *data_elem, **fields;

		for (i = 0; i < agg->aggregate.nr; i++) {
			struct ast_node *mnode = agg->aggregate.members[i]->node;
			size_t j, vs, va;
			struct ir_type **vfields, *variant;

			if (mnode->enum_member.nr_assoc == 0)
				continue;

			has_data = 1;

			vfields = arena_alloc(&ic->cc->arena, sizeof(*vfields) * mnode->enum_member.nr_assoc);

			for (j = 0; j < mnode->enum_member.nr_assoc; j++)
				vfields[j] = ir_sem_type_lowering(ic,
								  mnode->enum_member.assocs[j]->rtype);

			/*
			 * Build a "fake" struct with the enum member associated
			 * types and then get the alignment and size for this
			 * fake struct.
			 */
			variant = type_obj(ic, vfields, mnode->enum_member.nr_assoc);
			vs = ir_type_sizeof(variant);
			va = ir_type_alignof(variant);

			/*
			 * Payload array needs to be aligned to the most demanding
			 * alignment type.
			 */
			if (vs > max_size)
				max_size = vs;
			if (va > max_align)
				max_align = va;
		}

		if (!has_data)
			return ic->t_i32;
		/*
		 * NEEDSWORK: There is no i16 primitive type therefore there
		 * cannot be a 2 bytes alignment; if someday this prim type
		 * is added, this should change to check for 2 bytes alignment
		 *
		 * ---
		 *
		 * data_count is the number of array elements is needed to hold
		 * the data at a given alignment. NOT the number of payload
		 * elements. e.g.:
		 *
		 * [ i32, i32, i64] -> sizeof = 16; alignment = 8
		 * data_count = 2 -> 2 * 8 = 16; holds all the data
		 * but the number of elements is 3.
		 */
		if (max_align == 8) {
			data_elem = ic->t_i64;
			data_count = align_up(max_size, 8) / 8;
		} else if (max_align == 4) {
			data_elem = ic->t_i32;
			data_count = align_up(max_size, 4) / 4;
		} else {
			data_elem = ic->t_i8;
			data_count = max_size;
		}

		fields = arena_alloc(&ic->cc->arena, sizeof(*fields) * 2);
		fields[0] = ic->t_i32;
		fields[1] = type_arr(ic, data_elem, (long long)data_count);
		return type_obj(ic, fields, 2);
	}
	case TY_FN: {
		size_t i;
		struct ir_type **params;
		struct ir_type *ret;

		params = arena_alloc(&ic->cc->arena, sizeof(*params) * sem_type->fn.nr);
		for (i = 0; i < sem_type->fn.nr; i++)
			params[i] = ir_sem_type_lowering(ic, sem_type->fn.params[i]);

		ret = ir_sem_type_lowering(ic, sem_type->fn.ret);
		return type_fn(ic, params, sem_type->fn.nr, ret);
	}
	case TY_TUPLE: {
		size_t i;
		struct ir_type **fields;

		fields = arena_alloc(&ic->cc->arena,
				     sizeof(*fields) * sem_type->tuple.nr);
		for (i = 0; i < sem_type->tuple.nr; i++)
			fields[i] = ir_sem_type_lowering(ic, sem_type->tuple.elems[i]);

		return type_obj(ic, fields, sem_type->tuple.nr);
	}
	default:
		die("failed trying to lower semantic type to ir type");
	}
}

static struct ir_block *ir_build_block(struct ir_context *ic, const char *label)
{
	struct ir_block *b = arena_alloc(&ic->cc->arena, sizeof(*b));
	memset(b, 0, sizeof(*b));
	b->label = label;

	ARENA_ALLOC_GROW(&ic->cc->arena, ic->current_fn->blocks,
			 ic->current_fn->nr_block + 1,
			 ic->current_fn->alloc_block);
	ic->current_fn->blocks[ic->current_fn->nr_block++] = b;

	return b;
}

void ir_set_block(struct ir_context *ic, struct ir_block *block)
{
	ic->current_block = block;
}

static struct ir_function *ir_build_fn(struct ir_context *ic, struct ast_node *node)
{
	struct ir_function *fn;
	struct symbol *sym = node->rsym;
	size_t i;

	fn = arena_alloc(&ic->cc->arena, sizeof(*fn));
	memset(fn, 0, sizeof(*fn));

	fn->name = sym->tok.lex;
	fn->name_len = sym->tok.len;
	fn->ret_type = ir_sem_type_lowering(ic, sym->type->fn.ret);

	if (sym->fn.nr_param) {
		fn->params = arena_alloc(&ic->cc->arena,
					 sizeof(*fn->params) * sym->fn.nr_param);
		fn->nr_param = sym->fn.nr_param;

		for (i = 0; i < sym->fn.nr_param; i++) {
			fn->params[i].sid = ic->next_id++;
			fn->params[i].type = ir_sem_type_lowering(ic,
								  sym->fn.params[i]->type);
			fn->params[i].def = NULL;
		}
	}

	ARENA_ALLOC_GROW(&ic->cc->arena, ic->module->functions,
			 ic->module->nr_fn + 1, ic->module->alloc_fn);
	ic->module->functions[ic->module->nr_fn++] = fn;

	ic->current_fn = fn;
	ic->next_id = fn->nr_param;

	fn->entry = ir_build_block(ic, "entry");
	ic->current_block = fn->entry;

	return fn;
}

void ir_emit_jump(struct ir_context *ic, struct ir_block *target)
{
	struct ir_inst *inst = arena_alloc(&ic->cc->arena, sizeof(*inst));
	memset(inst, 0, sizeof(*inst));
	inst->op = IR_JUMP;
	inst->res = NULL;
	inst->parent = ic->current_block;

	ARENA_ALLOC_GROW(&ic->cc->arena, ic->current_block->insts,
			 ic->current_block->nr_inst + 1,
			 ic->current_block->alloc_inst);
	ic->current_block->insts[ic->current_block->nr_inst++] = inst;

	ARENA_ALLOC_GROW(&ic->cc->arena, ic->current_block->succs,
			 ic->current_block->nr_succ + 1,
			 ic->current_block->alloc_succ);
	ic->current_block->succs[ic->current_block->nr_succ++] = target;

	ARENA_ALLOC_GROW(&ic->cc->arena, target->preds,
			 target->nr_pred + 1,
			 target->alloc_pred);
	target->preds[target->nr_pred++] = ic->current_block;
}

void ir_emit_cjump(struct ir_context *ic, struct ir_operand *cond,
		   struct ir_block *then_b, struct ir_block *else_b)
{
	struct ir_inst *inst = arena_alloc(&ic->cc->arena, sizeof(*inst));
	memset(inst, 0, sizeof(*inst));
	inst->op = IR_CJUMP;
	inst->res = NULL;
	inst->operands = arena_alloc(&ic->cc->arena, sizeof(*inst->operands));
	inst->operands[0] = cond;
	inst->nr_operand = 1;
	inst->parent = ic->current_block;

	ARENA_ALLOC_GROW(&ic->cc->arena, ic->current_block->insts,
			 ic->current_block->nr_inst + 1,
			 ic->current_block->alloc_inst);
	ic->current_block->insts[ic->current_block->nr_inst++] = inst;

	ARENA_ALLOC_GROW(&ic->cc->arena, ic->current_block->succs,
			 ic->current_block->nr_succ + 2,
			 ic->current_block->alloc_succ);
	ic->current_block->succs[ic->current_block->nr_succ++] = then_b;
	ic->current_block->succs[ic->current_block->nr_succ++] = else_b;

	ARENA_ALLOC_GROW(&ic->cc->arena, then_b->preds,
			 then_b->nr_pred + 1, then_b->alloc_pred);
	then_b->preds[then_b->nr_pred++] = ic->current_block;

	ARENA_ALLOC_GROW(&ic->cc->arena, else_b->preds,
			 else_b->nr_pred + 1, else_b->alloc_pred);
	else_b->preds[else_b->nr_pred++] = ic->current_block;
}

static struct ir_operand *ir_emit_const_int(struct ir_context *ic, long long val,
					    struct ir_type *type)
{
	struct ir_inst *inst = arena_alloc(&ic->cc->arena, sizeof(*inst));
	memset(inst, 0, sizeof(*inst));

	inst->op = IR_CONST;
	inst->parent = ic->current_block;

	inst->res = arena_alloc(&ic->cc->arena, sizeof(*inst->res));
	inst->res->sid = ic->next_id++;
	inst->res->type = type;
	inst->res->def = inst;

	inst->imm = val;

	ARENA_ALLOC_GROW(&ic->cc->arena, ic->current_block->insts,
			 ic->current_block->nr_inst + 1,
			 ic->current_block->alloc_inst);
	ic->current_block->insts[ic->current_block->nr_inst++] = inst;

	return inst->res;
}

static void ir_emit_ret(struct ir_context *ic, struct ir_operand *val)
{
	struct ir_inst *inst = arena_alloc(&ic->cc->arena, sizeof(*inst));
	memset(inst, 0, sizeof(*inst));

	inst->op = IR_RET;
	inst->parent = ic->current_block;
	inst->res = NULL;

	if (val) {
		inst->operands = arena_alloc(&ic->cc->arena, sizeof(*inst->operands));
		inst->operands[0] = val;
		inst->nr_operand = 1;
	}

	ARENA_ALLOC_GROW(&ic->cc->arena, ic->current_block->insts,
			 ic->current_block->nr_inst + 1,
			 ic->current_block->alloc_inst);
	ic->current_block->insts[ic->current_block->nr_inst++] = inst;
}

static struct ir_operand *ir_emit_cast(struct ir_context *ic,
				       struct ir_operand *operand,
				       struct ir_type *type)
{
	struct ir_inst *inst = arena_alloc(&ic->cc->arena, sizeof(&inst));
	memset(inst, 0, sizeof(*inst));

	inst->op = IR_CAST;
	inst->parent = ic->current_block;

	inst->res = arena_alloc(&ic->cc->arena, sizeof(*inst->res));
	inst->res->sid = ic->next_id++;
	inst->res->type = type;
	inst->res->def = inst;

	inst->operands = arena_alloc(&ic->cc->arena, sizeof(*inst->operands));
	inst->operands[0] = operand;
	inst->nr_operand = 1;

	ARENA_ALLOC_GROW(&ic->cc->arena, ic->current_block->insts,
			 ic->current_block->nr_inst + 1,
			 ic->current_block->alloc_inst);
	ic->current_block->insts[ic->current_block->nr_inst++] = inst;

	return inst->res;
}

static struct ir_operand *ir_emit_binop(struct ir_context *ic, enum ir_op op,
					struct ir_operand *lhs, struct ir_operand *rhs,
					struct ir_type *res_type)
{
	struct ir_inst *inst = arena_alloc(&ic->cc->arena, sizeof(*inst));
	memset(inst, 0, sizeof(*inst));

	inst->op = op;
	inst->parent = ic->current_block;

	inst->res = arena_alloc(&ic->cc->arena, sizeof(*inst->res));
	inst->res->sid = ic->next_id++;
	inst->res->type = res_type;
	inst->res->def = inst;

	inst->operands = arena_alloc(&ic->cc->arena, sizeof(*inst->operands) * 2);
	inst->operands[0] = lhs;
	inst->operands[1] = rhs;
	inst->nr_operand = 2;

	ARENA_ALLOC_GROW(&ic->cc->arena, ic->current_block->insts,
			 ic->current_block->nr_inst + 1,
			 ic->current_block->alloc_inst);
	ic->current_block->insts[ic->current_block->nr_inst++] = inst;

	return inst->res;
}

static struct ir_operand *ir_emit_alloc(struct ir_context *ic, struct ir_type *type)
{
	struct ir_inst *inst = arena_alloc(&ic->cc->arena, sizeof(*inst));
	memset(inst, 0, sizeof(*inst));

	inst->op = IR_ALLOC;
	inst->parent = ic->current_block;

	inst->res = arena_alloc(&ic->cc->arena, sizeof(*inst->res));
	inst->res->sid = ic->next_id++;
	inst->res->type = type;
	inst->res->def = inst;

	ARENA_ALLOC_GROW(&ic->cc->arena, ic->current_block->insts,
			 ic->current_block->nr_inst + 1,
			 ic->current_block->alloc_inst);
	ic->current_block->insts[ic->current_block->nr_inst++] = inst;

	return inst->res;
}

static struct ir_operand *ir_emit_load(struct ir_context *ic,
				       struct ir_operand *operand)
{
	struct ir_inst *inst = arena_alloc(&ic->cc->arena, sizeof(*inst));
	memset(inst, 0, sizeof(*inst));

	inst->op = IR_LOAD;
	inst->parent = ic->current_block;

	inst->res = arena_alloc(&ic->cc->arena, sizeof(*inst->res));
	inst->res->sid = ic->next_id++;
	inst->res->type = operand->type;
	inst->res->def = inst;

	inst->operands = arena_alloc(&ic->cc->arena, sizeof(*inst->operands));
	inst->operands[0] = operand;
	inst->nr_operand = 1;

	ARENA_ALLOC_GROW(&ic->cc->arena, ic->current_block->insts,
			 ic->current_block->nr_inst + 1,
			 ic->current_block->alloc_inst);
	ic->current_block->insts[ic->current_block->nr_inst++] = inst;

	return inst->res;
}

static void ir_emit_store(struct ir_context *ic, struct ir_operand *ptr,
			  struct ir_operand *val)
{
	struct ir_inst *inst = arena_alloc(&ic->cc->arena, sizeof(*inst));
	memset(inst, 0, sizeof(*inst));

	inst->op = IR_STORE;
	inst->parent = ic->current_block;
	inst->res = NULL;

	inst->operands = arena_alloc(&ic->cc->arena,
				     sizeof(*inst->operands) * 2);
	inst->operands[0] = ptr;
	inst->operands[1] = val;
	inst->nr_operand = 2;

	ARENA_ALLOC_GROW(&ic->cc->arena, ic->current_block->insts,
			 ic->current_block->nr_inst + 1,
			 ic->current_block->alloc_inst);
	ic->current_block->insts[ic->current_block->nr_inst++] = inst;
}

static struct ir_operand *lower_expr(struct ir_context *ic, struct ast_node *node)
{
	switch (node->type) {
	case NODE_INT:
		return ir_emit_const_int(ic, node->lit_int.val,
					 ir_sem_type_lowering(ic, node->rtype));
	case NODE_BINARY: {
		struct ir_operand *lhs = lower_expr(ic, node->binary.left);
		struct ir_operand *rhs = lower_expr(ic, node->binary.right);
		struct ir_type *res = ir_sem_type_lowering(ic, node->rtype);
		enum ir_op op = ast_op_to_ir(node->binary.type);

		if (!is_cmp_op(op)) {
			if (lhs->type != res)
				lhs = ir_emit_cast(ic, lhs, res);
			if (rhs->type != res)
				rhs = ir_emit_cast(ic, rhs, res);
		}

		return ir_emit_binop(ic, op, lhs, rhs, res);
	}
	case NODE_UNARY: {
		struct ir_type *res_type = ir_sem_type_lowering(ic, node->rtype);

		switch (node->unary.type) {
		case OP_NEG: {
			struct ir_operand *x = lower_expr(ic, node->unary.operand);
			struct ir_operand *zero = ir_emit_const_int(ic, 0, x->type);
			return ir_emit_binop(ic, IR_SUB, zero, x, res_type);
		}
		case OP_BIT_NOT: {
			struct ir_operand *x = lower_expr(ic, node->unary.operand);
			struct ir_operand *ones = ir_emit_const_int(ic, -1, x->type);
			return ir_emit_binop(ic, IR_XOR, x, ones, res_type);
		}
		case OP_NOT: {
			struct ir_operand *x = lower_expr(ic, node->unary.operand);
			struct ir_operand *zero = ir_emit_const_int(ic, 0, x->type);
			return ir_emit_binop(ic, IR_EQ, x, zero, ic->t_i1);
		}
		default:
			die("unhandled unary op in IR lowering: %d", node->unary.type);
		}
	}
	case NODE_ID: {
		struct ir_operand *slot = node->rsym->ir_slot;
		assert(slot && "id symbol should carry its slot");
		return ir_emit_load(ic, slot);
	}
	default:
		die("unhandled expr in IR lowering: %d", node->type);
	}
}

static void lower_stmt(struct ir_context *ic, struct ast_node *node)
{
	switch (node->type) {
	case NODE_RETURN: {
		struct ir_operand *val = NULL;
		if (node->return_stmt.expr)
			val = lower_expr(ic, node->return_stmt.expr);
		ir_emit_ret(ic, val);
		break;
	}
	case NODE_EXPR_STMT:
		lower_expr(ic, node->expr_stmt.expr);
		break;
	default:
		die("unhandled stmt in IR lowering: %d", node->type);
	}
}

static void lower_dec(struct ir_context *ic, struct ast_node *node)
{
	switch (node->type) {
	case NODE_CONST_DEC:
	case NODE_LET_DEC: {
		struct ir_type *type = ir_sem_type_lowering(ic, node->rsym->type);
		struct ir_operand *slot = ir_emit_alloc(ic, type);
		struct ir_operand *operand;

		node->rsym->ir_slot = slot;

		if (node->rsym->kind == SYM_VAR && node->let_dec.init) {
			operand = lower_expr(ic, node->let_dec.init);
			ir_emit_store(ic, slot, operand);
		} else if (node->rsym->kind == SYM_CONST && node->const_dec.init) {
			operand = lower_expr(ic, node->const_dec.init);
			ir_emit_store(ic, slot, operand);
		}

		break;
	}
	default:
		lower_stmt(ic, node);
		break;
	}
}

static void lower_fn(struct ir_context *ic, struct ast_node *node)
{
	size_t i;

	assert(node->fn_dec.body);
	ir_build_fn(ic, node);

	for (i = 0; i < node->fn_dec.body->block.nr; i++)
		lower_dec(ic, node->fn_dec.body->block.childs[i]);
}

void ir_lower(struct ir_context *ic, struct ast_node *program)
{
	size_t i;

	for (i = 0; i < program->block.nr; i++) {
		struct ast_node *node = program->block.childs[i];

		switch (node->type) {
		case NODE_FN_DEC:
			lower_fn(ic, node);
			break;
		default:
			break;
		}
	}
}

void ir_init(struct ir_context *ic, struct compiler_context *cc)
{
	memset(ic, 0, sizeof(*ic));
	ic->cc = cc;

	ic->module = arena_alloc(&ic->cc->arena, sizeof(*ic->module));
	memset(ic->module, 0, sizeof(*ic->module));

	ic->t_i1 = prim_new(ic, IR_I1);
	ic->t_i8 = prim_new(ic, IR_I8);
	ic->t_i32 = prim_new(ic, IR_I32);
	ic->t_i64 = prim_new(ic, IR_I64);
	ic->t_f32 = prim_new(ic, IR_F32);
	ic->t_f64 = prim_new(ic, IR_F64);
	ic->t_void = prim_new(ic, IR_VOID);
}

static const char *ir_type_str(struct ir_type *t)
{
	switch (t->kind) {
	case IR_I1:
		return "i1";
	case IR_I8:
		return "i8";
	case IR_I32:
		return "i32";
	case IR_I64:
		return "i64";
	case IR_F32:
		return "f32";
	case IR_F64:
		return "f64";
	case IR_VOID:
		return "void";
	case IR_PTR:
		return "ptr";
	case IR_ARR:
		return "arr";
	case IR_STRUCT:
		return "struct";
	case IR_FN:
		return "fn";
	default:
		die("unknown type at ir_type_str()");
	}
}

static const char *ir_op_str(enum ir_op op)
{
	switch (op) {
	case IR_ADD:
		return "add";
	case IR_SUB:
		return "sub";
	case IR_MUL:
		return "mul";
	case IR_DIV:
		return "div";
	case IR_MOD:
		return "mod";
	case IR_AND:
		return "and";
	case IR_OR:
		return "or";
	case IR_XOR:
		return "xor";
	case IR_SHL:
		return "shl";
	case IR_SHR:
		return "shr";
	case IR_EQ:
		return "eq";
	case IR_NEQ:
		return "neq";
	case IR_LT:
		return "lt";
	case IR_GT:
		return "gt";
	case IR_LE:
		return "le";
	case IR_GE:
		return "ge";
	default:
		return NULL;
	}
}

static void ir_dump_inst(struct ir_inst *inst)
{
	printf("    ");

	if (inst->res)
		printf("%%%u = ", inst->res->sid);

	switch (inst->op) {
	case IR_CONST:
		printf("const %s %lld",
		       ir_type_str(inst->res->type), inst->imm);
		break;
	case IR_RET:
		if (inst->nr_operand)
			printf("ret %s %%%u",
			       ir_type_str(inst->operands[0]->type),
			       inst->operands[0]->sid);
		else
			printf("ret void");
		break;
	case IR_ALLOC:
		printf("alloc %s", ir_type_str(inst->res->type));
		break;
	case IR_LOAD:
		printf("load %s %%%u",
		       ir_type_str(inst->res->type),
		       inst->operands[0]->sid);
		break;
	case IR_STORE:
		printf("store %s %%%u, %%%u",
		       ir_type_str(inst->operands[1]->type),
		       inst->operands[1]->sid,
		       inst->operands[0]->sid);
		break;
	case IR_GEP:
		printf("gep %s %%%u, %%%u",
		       ir_type_str(inst->res->type),
		       inst->operands[0]->sid,
		       inst->operands[1]->sid);
		break;
	case IR_JUMP:
		printf("jump %s",
		       inst->parent->succs[0]->label);
		break;
	case IR_CJUMP:
		printf("cjump %%%u, %s, %s",
		       inst->operands[0]->sid,
		       inst->parent->succs[0]->label,
		       inst->parent->succs[1]->label);
		break;
	case IR_CALL:
		printf("call %%%u(",
		       inst->operands[0]->sid);
		for (size_t i = 1; i < inst->nr_operand; i++) {
			if (i > 1)
				printf(", ");
			printf("%s %%%u",
			       ir_type_str(inst->operands[i]->type),
			       inst->operands[i]->sid);
		}
		printf(")");
		break;
	case IR_PARAM:
		printf("param %s %lld",
		       ir_type_str(inst->res->type), inst->imm);
		break;
	case IR_PHI:
		printf("phi %s", ir_type_str(inst->res->type));
		for (size_t i = 0; i < inst->nr_operand; i++)
			printf("%s[%%%u, %s]",
			       i ? ", " : " ",
			       inst->operands[i]->sid,
			       inst->parent->preds[i]->label);
		break;
	case IR_CAST:
		printf("cast %s %%%u to %s",
		       ir_type_str(inst->operands[0]->type),
		       inst->operands[0]->sid,
		       ir_type_str(inst->res->type));
		break;
	default: {
		const char *binop;
		binop = ir_op_str(inst->op);
		if (binop)
			printf("%s %s %%%u, %%%u",
			       binop,
			       ir_type_str(inst->res->type),
			       inst->operands[0]->sid,
			       inst->operands[1]->sid);
		else
			printf("UNKNOWN op=%d", inst->op);
	}
	}

	printf("\n");
}

static void ir_dump_block(struct ir_block *block)
{
	size_t i;

	printf("%s:\n", block->label);
	for (i = 0; i < block->nr_inst; i++)
		ir_dump_inst(block->insts[i]);
}

static void ir_dump_fn(struct ir_function *fn)
{
	size_t i;

	printf("fn %.*s(", (int)fn->name_len, fn->name);
	for (i = 0; i < fn->nr_param; i++) {
		if (i > 0)
			printf(", ");
		printf("%s %%%u", ir_type_str(fn->params[i].type),
		       fn->params[i].sid);
	}
	printf(") -> %s {\n", ir_type_str(fn->ret_type));

	for (i = 0; i < fn->nr_block; i++)
		ir_dump_block(fn->blocks[i]);

	printf("}\n\n");
}

void ir_dump(struct ir_module *module)
{
	size_t i;

	for (i = 0; i < module->nr_fn; i++)
		ir_dump_fn(module->functions[i]);
}
