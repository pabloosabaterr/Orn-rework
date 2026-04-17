#!/bin/sh

# shellcheck disable=SC1091,SC2016,SC2034
. "$(dirname "$0")/test-lib.sh"

test_expect_success 'duplicated functions error' '
	cat >input.orn <<-\EOF &&
	fn foo() {}
	fn foo() {}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: '\''foo'\'' has been already declared
	 --> input.orn:2:3
	   |
	 2 | fn foo() {}
	   |    ^~~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'self-referential struct via pointer' '
	cat >input.orn <<-\EOF &&
	struct Node {
		val: int;
		next: *Node;
	}
	EOF
	"$ORN" input.orn >actual &&
	cat >expect <<-\EOF &&
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'struct with duplicate field' '
	cat >input.orn <<-\EOF &&
	struct Bad {
		x: int;
		x: bool;
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: duplicated field '\''x'\''
	 --> input.orn:3:0
	   |
	 3 | x: bool;
	   | ^
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'struct with unknown type' '
	cat >input.orn <<-\EOF &&
	struct Bad {
		x: Nonexistent;
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: unknown type '\''Nonexistent'\''
	 --> input.orn:2:3
	   |
	 2 | x: Nonexistent;
	   |    ^~~~~~~~~~~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'cyclic type alias detected' '
	cat >input.orn <<-\EOF &&
	type A = B;
	type B = A;
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: cyclic type alias '\''A'\''
	 --> input.orn:1:5
	   |
	 1 | type A = B;
	   |      ^
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'valid type alias' '
	cat >input.orn <<-\EOF &&
	type Byte = unsigned;
	EOF
	"$ORN" input.orn >actual &&
	cat >expect <<-\EOF &&
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'struct references another struct' '
	cat >input.orn <<-\EOF &&
	struct Point { x: int; y: int; }
	struct Line { start: Point; end: Point; }
	EOF
	"$ORN" input.orn >actual &&
	cat >expect <<-\EOF &&
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'



test_done
