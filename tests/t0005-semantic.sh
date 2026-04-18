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

test_expect_success 'integer arithmetic' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		let a: int = 1 + 2;
		let b: int = a * 3;
		let c: double = 1 + 2.0;
	}
	EOF
	"$ORN" input.orn >actual &&
	cat >expect <<-\EOF &&
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'pointer ops and null' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		let x: int = 42;
		let p: *int = &x;
		let q: *int = p + 1;
		let d: int = q - p;
		let v: int = *p;
		let n: *int = null;
		let b: bool = n == null;
	}
	EOF
	"$ORN" input.orn >actual &&
	cat >expect <<-\EOF &&
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'valid lvalue assignments' '
	cat >input.orn <<-\EOF &&
	struct Point { x: int; y: int; }
	fn foo() {
		let x: int = 1;
		x = 2;
		let p = Point { x: 1, y: 2 };
		p.x = 3;
		let a = [1, 2, 3];
		a[0] = 4;
	}
	EOF
	"$ORN" input.orn >actual &&
	cat >expect <<-\EOF &&
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'break inside loop is valid' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		while true {
			break;
		}
	}
	EOF
	"$ORN" input.orn >actual &&
	cat >expect <<-\EOF &&
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'pointer is truthy in if' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		let p: *int = null;
		if p { let x: int = *p; }
	}
	EOF
	"$ORN" input.orn >actual &&
	cat >expect <<-\EOF &&
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'qualified enum and namespace method' '
	cat >input.orn <<-\EOF &&
	enum Color { Red, Green, Blue }
	struct Point { x: int; y: int; }
	impl Point {
		fn origin() -> Point {
			ret Point { x: 0, y: 0 };
		}
	}
	fn foo() {
		let c: Color = Color::Red;
		let p: Point = Point::origin();
	}
	EOF
	"$ORN" input.orn >actual &&
	cat >expect <<-\EOF &&
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'shadowing in inner scope' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		let x: int = 1;
		{ let x: bool = true; }
	}
	EOF
	"$ORN" input.orn >actual &&
	cat >expect <<-\EOF &&
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'full program' '
	cat >input.orn <<-\EOF &&
	type Coord = int;
	struct Point { x: Coord; y: Coord; }
	enum Direction { Up, Down, Left, Right }
	impl Point {
		fn origin() -> Point {
			ret Point { x: 0, y: 0 };
		}
	}
	const MAX: int = 100;
	fn step(p: Point, d: Direction) -> Point {
		let result = Point { x: p.x, y: p.y };
		ret result;
	}
	fn main() {
		let p = Point::origin();
		let d = Direction::Up;
		let q = step(p, d);
		for i in 0..10 {
			let s = step(q, d);
		}
		while true {
			break;
		}
		let x: int = 42;
		let f: double = x as double;
		let a = [1, 2, 3];
		let v: int = a[0];
	}
	EOF
	"$ORN" input.orn >actual &&
	cat >expect <<-\EOF &&
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'bool arithmetic is error' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		let x = true + 1;
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: invalid operands to '\''+'\'': '\''bool'\'' and '\''int'\''
	 --> input.orn:2:13
	   |
	 2 | let x = true + 1;
	   |              ^
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'undeclared identifier' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		let x: int = y;
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: undeclared identifier '\''y'\''
	 --> input.orn:2:13
	   |
	 2 | let x: int = y;
	   |              ^
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'type mismatch in let' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		let x: int = "hello";
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: type mismatch: '\''int'\'' vs '\''string'\''
	 --> input.orn:2:0
	   |
	 2 | let x: int = "hello";
	   | ^~~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'dereference non-pointer' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		let x: int = 42;
		let y = *x;
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: cannot dereference '\''int'\''
	 --> input.orn:3:8
	   |
	 3 | let y = *x;
	   |         ^
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'cannot take address of literal' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		let p = &42;
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: cannot take address of this expression
	 --> input.orn:2:8
	   |
	 2 | let p = &42;
	   |         ^
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'cannot assign to literal' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		42 = 10;
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: cannot assign to this expression
	 --> input.orn:2:3
	   |
	 2 | 42 = 10;
	   |    ^
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'wrong number of arguments' '
	cat >input.orn <<-\EOF &&
	fn add(a: int, b: int) -> int {}
	fn foo() {
		add(1);
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: expected 2 arguments but got 1
	 --> input.orn:3:3
	   |
	 3 | add(1);
	   |    ^
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'wrong argument type' '
	cat >input.orn <<-\EOF &&
	fn add(a: int, b: int) -> int {}
	fn foo() {
		add(1, "hello");
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: argument 2: expected '\''int'\'' but got '\''string'\''
	 --> input.orn:3:7
	   |
	 3 | add(1, "hello");
	   |        ^~~~~~~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'return type mismatch' '
	cat >input.orn <<-\EOF &&
	fn foo() -> int {
		ret "hello";
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: return type mismatch: expected '\''int'\'' but got '\''string'\''
	 --> input.orn:2:0
	   |
	 2 | ret "hello";
	   | ^~~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'non-void empty return' '
	cat >input.orn <<-\EOF &&
	fn foo() -> int {
		ret;
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: non-void function must return a value
	 --> input.orn:2:0
	   |
	 2 | ret;
	   | ^~~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'return inside defer' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		defer {
			ret;
		}
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: '\''return'\'' inside '\''defer'\''
	 --> input.orn:3:0
	   |
	 3 | ret;
	   | ^~~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'break outside loop' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		break;
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: '\''break'\'' outside of loop
	 --> input.orn:2:0
	   |
	 2 | break;
	   | ^~~~~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'continue outside loop' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		continue;
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: '\''continue'\'' outside of loop
	 --> input.orn:2:0
	   |
	 2 | continue;
	   | ^~~~~~~~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'if condition must be booleable' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		if 42 {}
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: condition must be a booleable type, got '\''int'\''
	 --> input.orn:2:3
	   |
	 2 | if 42 {}
	   |    ^~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'for iterator not visible outside' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		for i in 0..10 {}
		let x: int = i;
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: undeclared identifier '\''i'\''
	 --> input.orn:3:13
	   |
	 3 | let x: int = i;
	   |              ^
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'missing field in struct init' '
	cat >input.orn <<-\EOF &&
	struct Point { x: int; y: int; }
	fn foo() {
		let p = Point { x: 1 };
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: missing field '\''y'\'' in '\''Point'\''
	 --> input.orn:3:8
	   |
	 3 | let p = Point { x: 1 };
	   |         ^~~~~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'duplicate field in struct init' '
	cat >input.orn <<-\EOF &&
	struct Point { x: int; y: int; }
	fn foo() {
		let p = Point { x: 1, x: 2, y: 3 };
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: duplicate field '\''x'\'' in initializer
	 --> input.orn:3:22
	   |
	 3 | let p = Point { x: 1, x: 2, y: 3 };
	   |                       ^
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'no such field' '
	cat >input.orn <<-\EOF &&
	struct Point { x: int; y: int; }
	fn foo() {
		let p = Point { x: 1, y: 2 };
		let z = p.z;
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: no field '\''z'\'' in '\''Point'\''
	 --> input.orn:4:10
	   |
	 4 | let z = p.z;
	   |           ^
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'array size mismatch' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		let a: [int; 5] = [1, 2, 3];
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: array size mismatch: expected 5 but got 3
	 --> input.orn:2:0
	   |
	 2 | let a: [int; 5] = [1, 2, 3];
	   | ^~~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'array element mismatch' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		let a = [1, "hello", 3];
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: array element type mismatch: expected '\''int'\'' but got '\''string'\''
	 --> input.orn:2:12
	   |
	 2 | let a = [1, "hello", 3];
	   |             ^~~~~~~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'index non-array' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		let x: int = 42;
		let y = x[0];
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: cannot index '\''int'\''
	 --> input.orn:3:9
	   |
	 3 | let y = x[0];
	   |          ^
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'invalid cast' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		let x = true as int;
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: cannot cast '\''bool'\'' to '\''int'\''
	 --> input.orn:2:13
	   |
	 2 | let x = true as int;
	   |              ^~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'unknown enum member' '
	cat >input.orn <<-\EOF &&
	enum Color { Red, Green, Blue }
	fn foo() {
		let c = Color::Purple;
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: '\''Color'\'' has no member '\''Purple'\''
	 --> input.orn:3:15
	   |
	 3 | let c = Color::Purple;
	   |                ^~~~~~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'match pattern type mismatch' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		let x: int = 42;
		match (x) {
			"hello" => {}
			_ => {}
		}
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: pattern type '\''string'\'' doesn'\''t match subject type '\''int'\''
	 --> input.orn:4:0
	   |
	 4 | "hello" => {}
	   | ^~~~~~~
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_success 'variable not visible outside block' '
	cat >input.orn <<-\EOF &&
	fn foo() {
	{ let x: int = 1; }
	let y: int = x;
	}
	EOF
	test_must_fail "$ORN" input.orn >actual 2>err &&
	cat >expect <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: undeclared identifier '\''x'\''
	 --> input.orn:3:13
	   |
	 3 | let y: int = x;
	   |              ^
	EOF
	test_cmp expect actual &&
	test_cmp expect.err err
'

test_expect_failure '::Red resolves with type hint when ambiguous' '
	cat >input.orn <<-\EOF &&
	enum Color { Red, Green, Blue }
	enum Priority { Red, Yellow, Green }
	fn foo() {
	let c: Color = ::Red;
	}
	EOF
	"$ORN" input.orn >actual 2>/dev/null &&
	cat >expect <<-\EOF &&
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_failure 'match pattern bindings create variables' '
	cat >input.orn <<-\EOF &&
	enum Option { Some(int), None }
	fn foo() {
		let o = Option::Some(42);
		match (o) {
			Option::Some(x) => { let y: int = x; }
			Option::None => {}
		}
	}
	EOF
	"$ORN" input.orn >actual 2>/dev/null &&
	cat >expect <<-\EOF &&
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_failure 'missing return detected in non-void function' '
	cat >input.orn <<-\EOF &&
	fn foo() -> int {
		if true {
			ret 1;
		}
	}
	EOF
	test_must_fail "$ORN" input.orn 2>/dev/null
'

test_expect_failure 'match exhaustiveness on enum without wildcard' '
	cat >input.orn <<-\EOF &&
	enum Color { Red, Green, Blue }
	fn foo() {
		let c = Color::Red;
		match (c) {
			Color::Red => {}
			Color::Green => {}
		}
	}
	EOF
	test_must_fail "$ORN" input.orn 2>/dev/null
'

test_done
