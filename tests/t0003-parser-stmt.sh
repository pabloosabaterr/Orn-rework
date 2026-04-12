#!/bin/sh

# shellcheck disable=SC1091,SC2016,SC2034
. "$(dirname "$0")/test-lib.sh"

test_expect_success 'empty block' '
	echo "{}" >input.orn &&
	cat >expect <<-\EOF &&
	PROGRAM
	`-- BLOCK
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'block with statements' '
	cat >input.orn <<-\EOF &&
	{ 1; 2; }
	EOF
	cat >expect <<-\EOF &&
	PROGRAM
	`-- BLOCK
	    |-- EXPR_STMT
	    |   `-- INT 1
	    `-- EXPR_STMT
	        `-- INT 2
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'if statement' '
	cat >input.orn <<-\EOF &&
	if x { 1; }
	EOF
	cat >expect <<-\EOF &&
	PROGRAM
	`-- IF (1 branch)
	    |-- ID '\''x'\''
	    `-- BLOCK
	        `-- EXPR_STMT
	            `-- INT 1
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'if elif else' '
	cat >input.orn <<-\EOF &&
	if a { 1; } elif b { 2; } else { 3; }
	EOF
	cat >expect <<-\EOF &&
	PROGRAM
	`-- IF (2 branches)
	    |-- ID '\''a'\''
	    |-- BLOCK
	    |   `-- EXPR_STMT
	    |       `-- INT 1
	    |-- ID '\''b'\''
	    |-- BLOCK
	    |   `-- EXPR_STMT
	    |       `-- INT 2
	    `-- BLOCK
	        `-- EXPR_STMT
	            `-- INT 3
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'while statement' '
	cat >input.orn <<-\EOF &&
	while x { 1; }
	EOF
	cat >expect <<-\EOF &&
	PROGRAM
	`-- WHILE
	    |-- ID '\''x'\''
	    `-- BLOCK
	        `-- EXPR_STMT
	            `-- INT 1
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'for statement' '
	cat >input.orn <<-\EOF &&
	for i in 0..10 { 1; }
	EOF
	cat >expect <<-\EOF &&
	PROGRAM
	`-- FOR '\''i'\''
	    |-- BINARY (..)
	    |   |-- INT 0
	    |   `-- INT 10
	    `-- BLOCK
	        `-- EXPR_STMT
	            `-- INT 1
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'return with value' '
	echo "ret 42;" >input.orn &&
	cat >expect <<-\EOF &&
	PROGRAM
	`-- RETURN
	    `-- INT 42
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'return without value' '
	echo "ret;" >input.orn &&
	cat >expect <<-\EOF &&
	PROGRAM
	`-- RETURN
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'break and continue' '
	echo "break;" >input.orn &&
	cat >expect <<-\EOF &&
	PROGRAM
	`-- BREAK
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual &&
	echo "continue;" >input.orn &&
	cat >expect <<-\EOF &&
	PROGRAM
	`-- CONTINUE
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'defer statement' '
	cat >input.orn <<-\EOF &&
	defer { 1; }
	EOF
	cat >expect <<-\EOF &&
	PROGRAM
	`-- DEFER
	    `-- BLOCK
	        `-- EXPR_STMT
	            `-- INT 1
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'increment statement' '
	echo "i++;" >input.orn &&
	cat >expect <<-\EOF &&
	PROGRAM
	`-- INCDEC (++)
	    `-- ID '\''i'\''
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'match with literal and wildcard' '
	cat >input.orn <<-\EOF &&
	match (x) {
		1 => { 2; }
		_ => { 3; }
	}
	EOF
	cat >expect <<-\EOF &&
	PROGRAM
	`-- MATCH
	    |-- ID '\''x'\''
	    |-- MATCH_ARM
	    |   |-- MATCH_PATTERN
	    |   |   `-- INT 1
	    |   `-- BLOCK
	    |       `-- EXPR_STMT
	    |           `-- INT 2
	    `-- MATCH_ARM
	        |-- MATCH_PATTERN (wildcard)
	        `-- BLOCK
	            `-- EXPR_STMT
	                `-- INT 3
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'match with constructor pattern' '
	cat >input.orn <<-\EOF &&
	match (x) {
		Some(n) => { n; }
		None => { 0; }
	}
	EOF
	cat >expect <<-\EOF &&
	PROGRAM
	`-- MATCH
	    |-- ID '\''x'\''
	    |-- MATCH_ARM
	    |   |-- MATCH_PATTERN (bind: n)
	    |   |   `-- ID '\''Some'\''
	    |   `-- BLOCK
	    |       `-- EXPR_STMT
	    |           `-- ID '\''n'\''
	    `-- MATCH_ARM
	        |-- MATCH_PATTERN
	        |   `-- ID '\''None'\''
	        `-- BLOCK
	            `-- EXPR_STMT
	                `-- INT 0
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'expression statement' '
	echo "foo(1);" >input.orn &&
	cat >expect <<-\EOF &&
	PROGRAM
	`-- EXPR_STMT
	    `-- CALL
	        |-- ID '\''foo'\''
	        `-- INT 1
	EOF
	"$ORN" --dump-ast input.orn >actual &&
	test_cmp expect actual
'

test_done
