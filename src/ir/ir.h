#ifndef IR_H
#define IR_H

#include <stddef.h>

struct compiler_context;
struct ast_node;

enum ir_op {
	IR_ADD,
	IR_SUB,
	IR_MUL,
	IR_DIV,
	IR_MOD,
	IR_AND,
	IR_OR,
	IR_XOR,
	IR_SHL,
	IR_SHR,
	IR_EQ,
	IR_NEQ,
	IR_LT,
	IR_GT,
	IR_LE,
	IR_GE,
	IR_ALLOC,
	IR_LOAD,
	IR_STORE,
	IR_GEP,
	IR_JUMP,
	IR_CJUMP,
	IR_RET,
	IR_CONST,
	IR_PARAM,
	IR_CALL,
	IR_PHI,
	IR_CAST,
};

enum ir_kind {
	IR_I1,
	IR_I8,
	IR_I32,
	IR_I64,
	IR_F32,
	IR_F64,
	IR_VOID,
	IR_PTR,
	IR_ARR,
	IR_STRUCT,
	IR_FN,
};

struct ir_type {
	enum ir_kind kind;
	union {
		struct { struct ir_type *pointee; } ptr;
		struct {
			struct ir_type *elem;
			long long size;
		} arr;
		struct {
			struct ir_type **fields;
			size_t nr_fields;
		} obj;
		struct {
			struct ir_type **params;
			size_t nr_param;
			struct ir_type *ret;
		} fn;
	};
};

struct ir_operand {
	/*
	 * static id
	 */
	unsigned sid;
	struct ir_type *type;
	struct ir_inst *def;
};

struct ir_inst {
	enum ir_op op;
	long long imm;
	/*
	 * Output operand; can be NULL.
	 */
	struct ir_operand *res;
	/*
	 * Consumed operands; from NULL to n.
	 */
	struct ir_operand **operands;
	size_t nr_operand;
	struct ir_block *parent;

	size_t nr_phi;
};

struct ir_block {
	const char *label;
	struct ir_inst **insts;
	size_t nr_inst;
	size_t alloc_inst;

	struct ir_block **preds;
	size_t nr_pred;
	size_t alloc_pred;
	struct ir_block **succs;
	size_t nr_succ;
	size_t alloc_succ;
};

struct ir_function {
	const char *name;
	size_t name_len;
	struct ir_type *ret_type;
	/*
	 * Can be NULL; from NULL to n.
	 */
	struct ir_operand *params;
	size_t nr_param;

	struct ir_block **blocks;
	size_t nr_block;
	size_t alloc_block;
	struct ir_block *entry;
};

struct ir_module {
	struct ir_function **functions;
	size_t nr_fn;
	size_t alloc_fn;
};

struct ir_context {
	struct compiler_context *cc;
	size_t next_id;
	struct ir_module *module;

	struct ir_function *current_fn;
	struct ir_block *current_block;

	/* singleton */
	struct ir_type *t_i1;
	struct ir_type *t_i8;
	struct ir_type *t_i32;
	struct ir_type *t_i64;
	struct ir_type *t_f32;
	struct ir_type *t_f64;
	struct ir_type *t_void;

	struct ir_type **ptrs;
	size_t nr_ptr;
	size_t alloc_ptr;

	struct ir_type **arrs;
	size_t nr_arr;
	size_t alloc_arr;

	struct ir_type **objs;
	size_t nr_obj;
	size_t alloc_obj;

	struct ir_type **fns;
	size_t nr_fn;
	size_t alloc_fn;
};

void ir_init(struct ir_context *ic, struct compiler_context *cc);
void ir_dump(struct ir_module *module);
void ir_lower(struct ir_context *ic, struct ast_node *program);

#endif
