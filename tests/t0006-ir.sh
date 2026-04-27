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
		let a: float = 3.14;
		let b: bool = true;
		let c: *int = null;
		let d: double = 2.71828;
		let e: char = '\''x'\'';
	}
	EOF
	cat >expect <<-\EOF &&
	fn foo() -> void {
	entry:
	    %0 = alloc f32
	    %1 = const f64 3.140000
	    %2 = cast f64 %1 to f32
	    store f32 %2, %0
	    %3 = alloc i1
	    %4 = const i1 1
	    store i1 %4, %3
	    %5 = alloc ptr
	    %6 = const ptr 0
	    store ptr %6, %5
	    %7 = alloc f64
	    %8 = const f64 2.718280
	    store f64 %8, %7
	    %9 = alloc i8
	    %10 = const i8 120
	    store i8 %10, %9
	}

	Program compiled with 0 errors
	EOF
	"$ORN" --dump-ir input.orn >actual &&
	test_cmp expect actual
'

test_done
