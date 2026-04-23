#include "ir/ir.h"
#include "compiler.h"
#include "memory/arena.h"
#include "semantic/semantic.h"
#include <assert.h>
#include <string.h>

static struct ir_type *prim_new(struct ir_context *ic, enum ir_kind kind)
{
	struct ir_type *t = arena_alloc(&ic->cc->arena, sizeof(*t));
	memset(t, 0, sizeof(*t));
	t->kind = kind;
	return t;
}

void ir_init(struct ir_context *ic, struct compiler_context *cc)
{
	memset(ic, 0, sizeof(*ic));
	ic->cc = cc;

	ic->t_i1 = prim_new(ic, IR_I1);
	ic->t_i8 = prim_new(ic, IR_I8);
	ic->t_i32 = prim_new(ic, IR_I32);
	ic->t_i64 = prim_new(ic, IR_I64);
	ic->t_f32 = prim_new(ic, IR_F32);
	ic->t_f64 = prim_new(ic, IR_F64);
	ic->t_void = prim_new(ic, IR_VOID);
}
