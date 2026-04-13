#!/bin/sh

# shellcheck disable=SC1091,SC2016,SC2034
. "$(dirname "$0")/test-lib.sh"

test_expect_success 'function declaration' '
	cat >input.orn <<-\EOF &&
	fn add(a: int, b: int) -> int {
		ret a + b;
	}
	EOF
	cat >expect <<-\EOF &&
	PROGRAM
	`-- FN_DEC '\''add'\''
	    |-- PARAM '\''a'\''
	    |   `-- TYPE '\''int'\''
	    |-- PARAM '\''b'\''
	    |   `-- TYPE '\''int'\''
	    |-- TYPE '\''int'\''
	    `-- BLOCK
	        `-- RETURN
	            `-- BINARY (+)
	                |-- ID '\''a'\''
	                `-- ID '\''b'\''
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'function no return type' '
	cat >input.orn <<-\EOF &&
	fn greet() {
		1;
	}
	EOF
	cat >expect <<-\EOF &&
	PROGRAM
	`-- FN_DEC '\''greet'\''
	    `-- BLOCK
	        `-- EXPR_STMT
	            `-- INT 1
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'struct declaration' '
	cat >input.orn <<-\EOF &&
	struct Point {
		x: int;
		y: int;
	}
	EOF
	cat >expect <<-\EOF &&
	PROGRAM
	`-- STRUCT_DEC '\''Point'\''
	    |-- FIELD '\''x'\''
	    |   `-- TYPE '\''int'\''
	    `-- FIELD '\''y'\''
	        `-- TYPE '\''int'\''
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'impl declaration' '
	cat >input.orn <<-\EOF &&
	impl Vec {
		fn new() -> Vec {
			1;
		}
	}
	EOF
	cat >expect <<-\EOF &&
	PROGRAM
	`-- IMPL_DEC '\''Vec'\''
	    `-- FN_DEC '\''new'\''
	        |-- TYPE '\''Vec'\''
	        `-- BLOCK
	            `-- EXPR_STMT
	                `-- INT 1
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'enum plain' '
	cat >input.orn <<-\EOF &&
	enum Color {
		Red,
		Green,
		Blue,
	}
	EOF
	cat >expect <<-\EOF &&
	PROGRAM
	`-- ENUM_DEC '\''Color'\''
	    |-- ENUM_MEMBER '\''Red'\''
	    |-- ENUM_MEMBER '\''Green'\''
	    `-- ENUM_MEMBER '\''Blue'\''
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'enum with associated types' '
	cat >input.orn <<-\EOF &&
	enum Option {
		Some(int),
		None,
	}
	EOF
	cat >expect <<-\EOF &&
	PROGRAM
	`-- ENUM_DEC '\''Option'\''
	    |-- ENUM_MEMBER '\''Some'\''
	    |   `-- TYPE '\''int'\''
	    `-- ENUM_MEMBER '\''None'\''
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'enum with explicit values' '
	cat >input.orn <<-\EOF &&
	enum Flags {
		Read = 1,
		Write = 2,
	}
	EOF
	cat >expect <<-\EOF &&
	PROGRAM
	`-- ENUM_DEC '\''Flags'\''
	    |-- ENUM_MEMBER '\''Read'\''
	    |   `-- INT 1
	    `-- ENUM_MEMBER '\''Write'\''
	        `-- INT 2
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'type alias' '
	echo "type Ptr = *int;" >input.orn &&
	cat >expect <<-\EOF &&
	PROGRAM
	`-- TYPE_DEC '\''Ptr'\''
	    `-- TYPE_PTR
	        `-- TYPE '\''int'\''
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'let with type and init' '
	echo "let x: int = 42;" >input.orn &&
	cat >expect <<-\EOF &&
	PROGRAM
	`-- LET_DEC '\''x'\''
	    |-- TYPE '\''int'\''
	    `-- INT 42
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'let with type only' '
	echo "let x: int;" >input.orn &&
	cat >expect <<-\EOF &&
	PROGRAM
	`-- LET_DEC '\''x'\''
	    `-- TYPE '\''int'\''
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'let with init only' '
	echo "let x = 42;" >input.orn &&
	cat >expect <<-\EOF &&
	PROGRAM
	`-- LET_DEC '\''x'\''
	    `-- INT 42
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'let destructuring' '
	echo "let (a, b) = foo();" >input.orn &&
	cat >expect <<-\EOF &&
	PROGRAM
	`-- LET_DEC (a, b)
	    `-- CALL
	        `-- ID '\''foo'\''
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'const declaration' '
	echo "const PI: int = 3.14;" >input.orn &&
	cat >expect <<-\EOF &&
	PROGRAM
	`-- CONST_DEC '\''PI'\''
	    |-- TYPE '\''int'\''
	    `-- FLOAT 3.140000
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'import declaration' '
	cat >input.orn <<-\EOF &&
	import "std/io";
	EOF
	cat >expect <<-\EOF &&
	PROGRAM
	`-- IMPORT '\''std/io'\''
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'let without type or init fails' '
	echo "let x;" >input.orn &&
	test_must_fail "$ORN" --dump-ast input.orn 2>err &&
	grep "let declaration" err
'

test_done
