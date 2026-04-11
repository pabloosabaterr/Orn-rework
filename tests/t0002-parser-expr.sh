#!/bin/sh

# shellcheck disable=SC1091,SC2016,SC2034
. "$(dirname "$0")/test-lib.sh"

test_expect_success 'integer literals' '
	echo "42" >input.orn &&
	cat >expect <<-\EOF &&
	INT 42
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'hex octal binary literals' '
	echo "0xFF" >input.orn &&
	cat >expect <<-\EOF &&
	INT 255
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual &&
	echo "0o77" >input.orn &&
	cat >expect <<-\EOF &&
	INT 63
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual &&
	echo "0b1010" >input.orn &&
	cat >expect <<-\EOF &&
	INT 10
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'float literal' '
	echo "3.14" >input.orn &&
	cat >expect <<-\EOF &&
	FLOAT 3.140000
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'string literal' '
	cat >input.orn <<-\EOF &&
	"hello"
	EOF
	cat >expect <<-\EOF &&
	STRING '\''hello'\''
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'bool and null literals' '
	echo "true" >input.orn &&
	cat >expect <<-\EOF &&
	BOOL true
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual &&
	echo "null" >input.orn &&
	cat >expect <<-\EOF &&
	NULL
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'precedence mul over add' '
	echo "1 + 2 * 3" >input.orn &&
	cat >expect <<-\EOF &&
	BINARY (+)
	|-- INT 1
	`-- BINARY (*)
	    |-- INT 2
	    `-- INT 3
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'left associativity' '
	echo "1 + 2 + 3" >input.orn &&
	cat >expect <<-\EOF &&
	BINARY (+)
	|-- BINARY (+)
	|   |-- INT 1
	|   `-- INT 2
	`-- INT 3
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'right associativity of assign' '
	echo "a = b = 1" >input.orn &&
	cat >expect <<-\EOF &&
	ASSIGN (=)
	|-- ID '\''a'\''
	`-- ASSIGN (=)
	    |-- ID '\''b'\''
	    `-- INT 1
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'unary negation' '
	echo "-1" >input.orn &&
	cat >expect <<-\EOF &&
	UNARY (-)
	`-- INT 1
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'function call' '
	echo "foo(1, 2)" >input.orn &&
	cat >expect <<-\EOF &&
	CALL
	|-- ID '\''foo'\''
	|-- INT 1
	`-- INT 2
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'member access' '
	echo "a.b.c" >input.orn &&
	cat >expect <<-\EOF &&
	MEMBER '\''c'\''
	`-- MEMBER '\''b'\''
	    `-- ID '\''a'\''
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'namespace access' '
	echo "Foo::bar" >input.orn &&
	cat >expect <<-\EOF &&
	NAMESPACE '\''bar'\''
	`-- ID '\''Foo'\''
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'index access' '
	echo "a[0]" >input.orn &&
	cat >expect <<-\EOF &&
	INDEX
	|-- ID '\''a'\''
	`-- INT 0
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'cast expression' '
	echo "x as int" >input.orn &&
	cat >expect <<-\EOF &&
	CAST
	|-- ID '\''x'\''
	`-- TYPE '\''int'\''
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'sizeof' '
	echo "sizeof(int)" >input.orn &&
	cat >expect <<-\EOF &&
	SIZEOF
	`-- TYPE '\''int'\''
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'struct init' '
	echo "Point { x: 1, y: 2 }" >input.orn &&
	cat >expect <<-\EOF &&
	STRUCT_INIT (2 items)
	|-- FIELD_INIT '\''x'\''
	|   `-- INT 1
	`-- FIELD_INIT '\''y'\''
	    `-- INT 2
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'array init' '
	echo "[1, 2, 3]" >input.orn &&
	cat >expect <<-\EOF &&
	ARRAY_INIT (3 items)
	|-- INT 1
	|-- INT 2
	`-- INT 3
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'parenthesized expression' '
	echo "(1 + 2) * 3" >input.orn &&
	cat >expect <<-\EOF &&
	BINARY (*)
	|-- BINARY (+)
	|   |-- INT 1
	|   `-- INT 2
	`-- INT 3
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'syscall' '
	echo "syscall(1, 2, 3)" >input.orn &&
	cat >expect <<-\EOF &&
	SYSCALL (3 items)
	|-- INT 1
	|-- INT 2
	`-- INT 3
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_done
