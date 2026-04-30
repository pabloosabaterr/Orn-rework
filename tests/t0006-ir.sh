#!/bin/sh

# shellcheck disable=SC1091,SC2016,SC2034
. "$(dirname "$0")/test-lib.sh"

test_expect_success 'function returning constant' '
	cat >input.orn <<-\EOF &&
	fn five() -> int {
		ret 5;
	}
	EOF
	cat >expect <<-\EOF &&
	fn five() -> i32 {
	entry:
	    %0 = const i32 5
	    ret i32 %0
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'binary operations' '
	cat >input.orn <<-\EOF &&
	fn arith() -> int {
	ret (10 - 3) * 2 + 1;
	}

	fn cmp() -> bool {
		ret 5 > 3;
	}
	EOF
	cat >expect <<-\EOF &&
	fn arith() -> i32 {
	entry:
	    %0 = const i32 10
	    %1 = const i32 3
	    %2 = sub i32 %0, %1
	    %3 = const i32 2
	    %4 = mul i32 %2, %3
	    %5 = const i32 1
	    %6 = add i32 %4, %5
	    ret i32 %6
	}

	fn cmp() -> i1 {
	entry:
	    %0 = const i32 5
	    %1 = const i32 3
	    %2 = gt i1 %0, %1
	    ret i1 %2
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'unary operation' '
	cat >input.orn <<-\EOF &&
	fn neg() -> int {
		ret -5;
	}
	EOF
	cat >expect <<-\EOF &&
	fn neg() -> i32 {
	entry:
	    %0 = const i32 5
	    %1 = const i32 0
	    %2 = sub i32 %1, %0
	    ret i32 %2
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'let declaration, store and load' '
	cat >input.orn <<-\EOF &&
	fn foo() -> int {
		let x: int = 2;
		ret x;
	}
	EOF
	cat >expect <<-\EOF &&
	fn foo() -> i32 {
	entry:
	    %0 = alloc i32
	    %1 = const i32 2
	    store i32 %1, %0
	    %2 = load i32 %0
	    ret i32 %2
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'numeric types except int' '
	cat >input.orn <<-\EOF &&
	fn foo() -> void {
		let a: int = 10;
		a += 5;
	}
	EOF
	cat >expect <<-\EOF &&
	fn foo() -> void {
	entry:
	    %0 = alloc i32
	    %1 = const i32 10
	    store i32 %1, %0
	    %2 = const i32 5
	    %3 = load i32 %0
	    %4 = add i32 %3, %2
	    store i32 %4, %0
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'numeric types except int' '
	cat >input.orn <<-\EOF &&
	fn foo() -> void {
		let a: int = 10;
		a++;
		a--;
	}
	EOF
	cat >expect <<-\EOF &&
	fn foo() -> void {
	entry:
	    %0 = alloc i32
	    %1 = const i32 10
	    store i32 %1, %0
	    %2 = load i32 %0
	    %3 = const i32 1
	    %4 = add i32 %2, %3
	    store i32 %4, %0
	    %5 = load i32 %0
	    %6 = const i32 1
	    %7 = sub i32 %5, %6
	    store i32 %7, %0
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'flow branches' '
	cat >input.orn <<-\EOF &&
	fn foo() -> int {
		let x: int = 10;
		if x > 0 {
			x++;
		} else {
			x--;
		}
		ret x;
	}
	EOF
	cat >expect <<-\EOF &&
	fn foo() -> i32 {
	entry:
	    %0 = alloc i32
	    %1 = const i32 10
	    store i32 %1, %0
	    %2 = load i32 %0
	    %3 = const i32 0
	    %4 = gt i1 %2, %3
	    cjump %4, then, else
	then:
	    %5 = load i32 %0
	    %6 = const i32 1
	    %7 = add i32 %5, %6
	    store i32 %7, %0
	    jump merge
	else:
	    %8 = load i32 %0
	    %9 = const i32 1
	    %10 = sub i32 %8, %9
	    store i32 %10, %0
	    jump merge
	merge:
	    %11 = load i32 %0
	    ret i32 %11
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'for loop' '
	cat >input.orn <<-\EOF &&
	fn foo() -> int {
		true;
		for i in 0..10 {
			true;
		}
		true;
	}
	EOF
	cat >expect <<-\EOF &&
	fn foo() -> i32 {
	entry:
	    %0 = const i1 1
	    jump init
	init:
	    %1 = alloc i32
	    %2 = const i32 0
	    store i32 %2, %1
	    jump cond
	cond:
	    %3 = load i32 %1
	    %4 = const i32 10
	    %5 = lt i1 %3, %4
	    cjump %5, body, exit
	body:
	    %6 = const i1 1
	    jump it
	it:
	    %7 = load i32 %1
	    %8 = const i32 1
	    %9 = add i32 %7, %8
	    store i32 %9, %1
	    jump cond
	exit:
	    %10 = const i1 1
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'while loop' '
	cat >input.orn <<-\EOF &&
	fn foo() -> int {
		true;
		while true {
			true;
		}
		true;
	}
	EOF
	cat >expect <<-\EOF &&
	fn foo() -> i32 {
	entry:
	    %0 = const i1 1
	    jump cond
	cond:
	    %1 = const i1 1
	    cjump %1, body, exit
	body:
	    %2 = const i1 1
	    jump cond
	exit:
	    %3 = const i1 1
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'function call and function params' '
	cat >input.orn <<-\EOF &&
	fn add(a: int, b: int) -> int {
		ret a + b;
	}

	fn main() -> int {
		ret add(3, 4);
	}
	EOF
	cat >expect <<-\EOF &&
	fn add(i32 %0, i32 %1) -> i32 {
	entry:
	    %2 = alloc i32
	    store i32 %0, %2
	    %3 = alloc i32
	    store i32 %1, %3
	    %4 = load i32 %2
	    %5 = load i32 %3
	    %6 = add i32 %4, %5
	    ret i32 %6
	}

	fn main() -> i32 {
	entry:
	    %0 = const i32 3
	    %1 = const i32 4
	    %2 = call add(i32 %0, i32 %1)
	    ret i32 %2
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'array init' '
	cat >input.orn <<-\EOF &&
	fn main() -> int {
		let arr: [int; 3] = [1, 2, 3];
		ret arr[1];
	}
	EOF
	cat >expect <<-\EOF &&
	fn main() -> i32 {
	entry:
	    %0 = alloc arr
	    %1 = const i32 0
	    %2 = gep i32 %0, %1
	    %3 = const i32 1
	    store i32 %3, %2
	    %4 = const i32 1
	    %5 = gep i32 %0, %4
	    %6 = const i32 2
	    store i32 %6, %5
	    %7 = const i32 2
	    %8 = gep i32 %0, %7
	    %9 = const i32 3
	    store i32 %9, %8
	    %10 = const i32 1
	    %11 = gep i32 %0, %10
	    %12 = load i32 %11
	    ret i32 %12
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'struct init and member access' '
	cat >input.orn <<-\EOF &&
	struct Point {
		x: int;
		y: double;
	}

	fn main() -> double {
		let obj: Point = Point { x: 1, y: 2.0 };
		ret obj.y;
	}
	EOF
	cat >expect <<-\EOF &&
	fn main() -> f64 {
	entry:
	    %0 = alloc struct
	    %1 = const i32 0
	    %2 = gep i32 %0, %1
	    %3 = const i32 1
	    store i32 %3, %2
	    %4 = const i32 1
	    %5 = gep f64 %0, %4
	    %6 = const f64 2.000000
	    store f64 %6, %5
	    %7 = const i32 1
	    %8 = gep f64 %0, %7
	    %9 = load f64 %8
	    ret f64 %9
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'enum members' '
	cat >input.orn <<-\EOF &&
	enum test {
		A = 1,
		B(int),
		C,
	}

	fn foo() {
		let a: test = test::A;
		let b: test = test::B(42);
		let c: test = test::C;
	}
	EOF
	cat >expect <<-\EOF &&
	fn foo() -> void {
	entry:
	    %0 = alloc struct
	    %1 = const i32 1
	    %2 = const i32 0
	    %3 = gep i32 %0, %2
	    store i32 %1, %3
	    %4 = alloc struct
	    %5 = const i32 2
	    %6 = const i32 0
	    %7 = gep i32 %4, %6
	    store i32 %5, %7
	    %8 = const i32 1
	    %9 = gep arr %4, %8
	    %10 = const i32 42
	    %11 = const i32 0
	    %12 = gep i32 %9, %11
	    store i32 %10, %12
	    %13 = alloc struct
	    %14 = const i32 3
	    %15 = const i32 0
	    %16 = gep i32 %13, %15
	    store i32 %14, %16
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'enum without tagged union' '
	cat >input.orn <<-\EOF &&
	enum test { A, B }
	fn foo() {
		let a: test = ::A;
	}
	EOF
	cat >expect <<-\EOF &&
	fn foo() -> void {
	entry:
	    %0 = alloc i32
	    %1 = const i32 0
	    store i32 %1, %0
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'enum member as expression' '
	cat >input.orn <<-\EOF &&
	enum test { A, B }
	fn foo()-> test {
		ret ::B;
	}
	EOF
	cat >expect <<-\EOF &&
	fn foo() -> i32 {
	entry:
	    %0 = const i32 1
	    ret i32 %0
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'struct field assignment' '
	cat >input.orn <<-\EOF &&
	struct A { y: int; b: int; }
	fn foo() {
		let a: A = A { y: 1, b: 2 };
		a.y = 3;
	}
	EOF
	cat >expect <<-\EOF &&
	fn foo() -> void {
	entry:
	    %0 = alloc struct
	    %1 = const i32 0
	    %2 = gep i32 %0, %1
	    %3 = const i32 1
	    store i32 %3, %2
	    %4 = const i32 1
	    %5 = gep i32 %0, %4
	    %6 = const i32 2
	    store i32 %6, %5
	    %7 = const i32 0
	    %8 = gep i32 %0, %7
	    %9 = const i32 3
	    store i32 %9, %8
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'array index assignement' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		let b: [int; 3] = [1, 2, 3];
		b[0] = 4;
	}
	EOF
	cat >expect <<-\EOF &&
	fn foo() -> void {
	entry:
	    %0 = alloc arr
	    %1 = const i32 0
	    %2 = gep i32 %0, %1
	    %3 = const i32 1
	    store i32 %3, %2
	    %4 = const i32 1
	    %5 = gep i32 %0, %4
	    %6 = const i32 2
	    store i32 %6, %5
	    %7 = const i32 2
	    %8 = gep i32 %0, %7
	    %9 = const i32 3
	    store i32 %9, %8
	    %10 = const i32 0
	    %11 = gep i32 %0, %10
	    %12 = const i32 4
	    store i32 %12, %11
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'addr' '
	cat >input.orn <<-\EOF &&
	struct B { x: int; }
	fn foo() {
		let a: B = B { x: 10 };
		let p: *int = &a.x;
	}
	EOF
	cat >expect <<-\EOF &&
	fn foo() -> void {
	entry:
	    %0 = alloc struct
	    %1 = const i32 0
	    %2 = gep i32 %0, %1
	    %3 = const i32 10
	    store i32 %3, %2
	    %4 = alloc ptr
	    %5 = const i32 0
	    %6 = gep ptr %0, %5
	    store ptr %6, %4
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_expect_success 'deref asssign' '
	cat >input.orn <<-\EOF &&
	fn test_deref() {
		let x: int = 42;
		let p: *int = &x;
		*p = 99;
	}
	EOF
	cat >expect <<-\EOF &&
	fn test_deref() -> void {
	entry:
	    %0 = alloc ptr
	    %1 = const i32 42
	    store i32 %1, %0
	    %2 = alloc ptr
	    store ptr %0, %2
	    %3 = load i32 %2
	    %4 = const i32 99
	    store i32 %4, %3
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_done
