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

test_expect_success 'function param with unknown type' '
	cat >input.orn <<-\EOF &&
	fn bad(x: Nope) {}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: unknown type '\''Nope'\''
	 --> input.orn:1:10
	   |
	 1 | fn bad(x: Nope) {}
	   |           ^~~~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'function return unknown type' '
	cat >input.orn <<-\EOF &&
	fn bad() -> Nope {}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: unknown type '\''Nope'\''
	 --> input.orn:1:12
	   |
	 1 | fn bad() -> Nope {}
	   |             ^~~~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'impl on unknown type' '
	cat >input.orn <<-\EOF &&
	impl Ghost {
		fn spook() {}
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: impl for unknown type '\''Ghost'\''
	 --> input.orn:1:5
	   |
	 1 | impl Ghost {
	   |      ^~~~~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'impl on non-struct non-enum' '
	cat >input.orn <<-\EOF &&
	fn foo() {}
	impl foo {
		fn bar() {}
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: cannot impl '\''foo'\'': not a struct or enum
	 --> input.orn:2:5
	   |
	 2 | impl foo {
	   |      ^~~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'duplicate method in impl' '
	cat >input.orn <<-\EOF &&
	struct Point { x: int; y: int; }
	impl Point {
		fn origin() -> Point {}
		fn origin() -> Point {}
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: duplicate method '\''origin'\'' in '\''Point'\''
	 --> input.orn:4:3
	   |
	 4 | fn origin() -> Point {}
	   |    ^~~~~~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'method name collides with field' '
	cat >input.orn <<-\EOF &&
	struct Point { x: int; }
	impl Point {
		fn x() -> int {}
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: duplicate method '\''x'\'' in '\''Point'\''
	 --> input.orn:3:3
	   |
	 3 | fn x() -> int {}
	   |    ^
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'function uses struct, enum and alias' '
	cat >input.orn <<-\EOF &&
	type Coord = int;
	struct Point { x: Coord; y: Coord; }
	enum Color { Red, Green, Blue }
	fn paint(p: Point, c: Color) -> bool {}
	EOF
	"$ORN" input.orn >actual &&
	cat >expect <<-\EOF &&
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'impl with multiple methods and pointer params' '
	cat >input.orn <<-\EOF &&
	struct Node {
		val: int;
		next: *Node;
	}
	impl Node {
		fn create(v: int) -> Node {}
		fn value(self: *Node) -> int {}
		fn set_next(self: *Node, other: *Node) -> void {}
	}
	const MAX: int = 100;
	let counter: int = 0;
	EOF
	"$ORN" input.orn >actual &&
	cat >expect <<-\EOF &&
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_done
