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

test_done
