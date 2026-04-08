#!/bin/sh

# shellcheck disable=SC1091,SC2016,SC2034
. "$(dirname "$0")/test-lib.sh"

ORN="./build/orn"

test_expect_success 'basic tokenization' '
	cat >input.orn <<-\EOF &&
	fn main() {
		let x: int = 42;
	}
	EOF
	"$ORN" --dump-tokens input.orn >actual &&
	cat >expect <<-\EOF &&
	fn - 1:0
	main - 1:3
	( - 1:7
	) - 1:8
	{ - 1:10
	let - 2:0
	x - 2:4
	: - 2:5
	int - 2:7
	= - 2:11
	42 - 2:13
	; - 2:15
	} - 3:0
	EOF
	test_cmp expect actual
'

test_expect_success 'multi-character operators' '
	cat >input.orn <<-\EOF &&
	== != << >> -> <- ++ -- <= >= && ||
	EOF
	"$ORN" --dump-tokens input.orn >actual &&
	cat >expect <<-\EOF &&
	== - 1:0
	!= - 1:3
	<< - 1:6
	>> - 1:9
	-> - 1:12
	<- - 1:15
	++ - 1:18
	-- - 1:21
	<= - 1:24
	>= - 1:27
	&& - 1:30
	|| - 1:33
	EOF
	test_cmp expect actual
'

test_expect_success 'single-character operators' '
	cat >input.orn <<-\EOF &&
	+ - * / % ~ ^ ? : . , ;
	EOF
	"$ORN" --dump-tokens input.orn >actual &&
	cat >expect <<-\EOF &&
	+ - 1:0
	- - 1:2
	* - 1:4
	/ - 1:6
	% - 1:8
	~ - 1:10
	^ - 1:12
	? - 1:14
	: - 1:16
	. - 1:18
	, - 1:20
	; - 1:22
	EOF
	test_cmp expect actual
'

test_expect_success 'keywords are not identifiers' '
	cat >input.orn <<-\EOF &&
	if iff iffy return returning
	EOF
	"$ORN" --dump-tokens input.orn >actual &&
	cat >expect <<-\EOF &&
	if - 1:0
	iff - 1:3
	iffy - 1:7
	return - 1:12
	returning - 1:19
	EOF
	test_cmp expect actual
'

test_expect_success 'string and char literals' '
	cat >input.orn <<-\INPUT &&
	"hello" '\''c'\'' "world"
	INPUT
	"$ORN" --dump-tokens input.orn >actual &&
	cat >expect <<-\EOF &&
	"hello" - 1:0
	'\''c'\'' - 1:8
	"world" - 1:12
	EOF
	test_cmp expect actual
'

test_expect_success 'number literals' '
	cat >input.orn <<-\EOF &&
	0 42 12345
	EOF
	"$ORN" --dump-tokens input.orn >actual &&
	cat >expect <<-\EOF &&
	0 - 1:0
	42 - 1:2
	12345 - 1:5
	EOF
	test_cmp expect actual
'

test_expect_success 'line comments are skipped' '
	cat >input.orn <<-\EOF &&
	x // this is a comment
	y
	EOF
	"$ORN" --dump-tokens input.orn >actual &&
	cat >expect <<-\EOF &&
	x - 1:0
	y - 2:0
	EOF
	test_cmp expect actual
'

test_expect_success 'block comments are skipped' '
	cat >input.orn <<-\EOF &&
	x /* this is
	a block comment */ y
	EOF
	"$ORN" --dump-tokens input.orn >actual &&
	cat >expect <<-\EOF &&
	x - 1:0
	y - 2:19
	EOF
	test_cmp expect actual
'

test_expect_success 'empty input produces no tokens' '
	>input.orn &&
	"$ORN" --dump-tokens input.orn >actual &&
	test_must_be_empty actual
'

test_expect_success 'unexpected character reports error' '
	echo "@" >input.orn &&
	test_must_fail "$ORN" --dump-tokens input.orn 2>err &&
	grep "unexpected character" err
'

test_expect_success 'unterminated string reports error' '
	printf "\"\\n" >input.orn &&
	test_must_fail "$ORN" --dump-tokens input.orn 2>err &&
	grep "unterminated string" err
'

test_expect_success 'unterminated block comment reports error' '
	echo "/* oops" >input.orn &&
	test_must_fail "$ORN" --dump-tokens input.orn 2>err &&
	grep "unterminated" err
'

test_done
