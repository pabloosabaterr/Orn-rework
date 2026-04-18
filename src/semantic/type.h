#ifndef TYPE_H
#define TYPE_H

#include <stddef.h>

struct symbol;

enum base_type {
	TY_ERROR,

	TY_INT,
	TY_UINT,
	TY_FLOAT,
	TY_DOUBLE,
	TY_BOOL,
	TY_CHAR,
	TY_STRING,
	TY_VOID,
	TY_NULL,
	TY_PTR,
	TY_ARR,
	TY_TUPLE,
	TY_STRUCT,
	TY_ENUM,
	TY_FN,
	TY_TYPE_ALIAS,
};

struct type {
	enum base_type kind;
	union {
		struct {
			struct type *pointee;
		} ptr;
		struct {
			struct type *elem_type;
			long long size;
		} arr;
		struct {
			struct type **elems;
			size_t nr;
		} tuple;
		struct {
			struct symbol *dec;
		} named;
		struct {
			struct symbol *dec;
			struct type *resolved;
		} alias;
		struct {
			struct type **params;
			size_t nr;
			struct type *ret;
			unsigned is_variadic:1;
		} fn;
	};
};

#endif
