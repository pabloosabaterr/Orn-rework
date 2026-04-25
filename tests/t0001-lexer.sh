#!/bin/sh

# shellcheck disable=SC1091,SC2016,SC2034
. "$(dirname "$0")/test-lib.sh"

test_expect_success 'basic tokenization' '
	cat >input.orn <<-\EOF &&
	fn main() {
		let x: int = 42;
	}
	EOF
	"$ORN" --dump-tokens input.orn >actual &&
	cat >expect <<-\EOF &&
	fn [fn] - 1:0
	main [ID] - 1:3
	( [LPAREN] - 1:7
	) [RPAREN] - 1:8
	{ [LBRACE] - 1:10
	let [let] - 2:0
	x [ID] - 2:4
	: [COLON] - 2:5
	int [int] - 2:7
	= [EQUAL] - 2:11
	42 [NUMBER] - 2:13
	; [SEMICOLON] - 2:15
	} [RBRACE] - 3:0
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'multi-character operators' '
	cat >input.orn <<-\EOF &&
	== != << >> -> <- ++ -- <= >= && ||
	EOF
	"$ORN" --dump-tokens input.orn >actual &&
	cat >expect <<-\EOF &&
	== [CMP] - 1:0
	!= [NEQ] - 1:3
	<< [LSHIFT] - 1:6
	>> [RSHIFT] - 1:9
	-> [RARROW] - 1:12
	<- [LARROW] - 1:15
	++ [INCREMENT] - 1:18
	-- [DECREMENT] - 1:21
	<= [LE] - 1:24
	>= [GE] - 1:27
	&& [AND] - 1:30
	|| [OR] - 1:33
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'single-character operators' '
	cat >input.orn <<-\EOF &&
	+ - * / % ~ ^ ? : . , ;
	EOF
	"$ORN" --dump-tokens input.orn >actual &&
	cat >expect <<-\EOF &&
	+ [PLUS] - 1:0
	- [MINUS] - 1:2
	* [STAR] - 1:4
	/ [SLASH] - 1:6
	% [MOD] - 1:8
	~ [TILDE] - 1:10
	^ [CARET] - 1:12
	? [QUESTION] - 1:14
	: [COLON] - 1:16
	. [DOT] - 1:18
	, [COMMA] - 1:20
	; [SEMICOLON] - 1:22
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'keywords are not identifiers' '
	cat >input.orn <<-\EOF &&
	if iff iffy return returning
	EOF
	"$ORN" --dump-tokens input.orn >actual &&
	cat >expect <<-\EOF &&
	if [if] - 1:0
	iff [ID] - 1:3
	iffy [ID] - 1:7
	return [ID] - 1:12
	returning [ID] - 1:19
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'string and char literals' '
	cat >input.orn <<-\INPUT &&
	"hello" '\''c'\'' "world"
	INPUT
	"$ORN" --dump-tokens input.orn >actual &&
	cat >expect <<-\EOF &&
	"hello" [STRING] - 1:0
	'\''c'\'' [CHAR] - 1:8
	"world" [STRING] - 1:12
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'number literals' '
	cat >input.orn <<-\EOF &&
	0 42 12345
	3.14 0.5 100.0
	0xFF 0XAB
	0o77 0O17
	0b1010 0B11
	EOF
	"$ORN" --dump-tokens input.orn >actual &&
	cat >expect <<-\EOF &&
	0 [NUMBER] - 1:0
	42 [NUMBER] - 1:2
	12345 [NUMBER] - 1:5
	3.14 [FLOATING] - 2:0
	0.5 [FLOATING] - 2:5
	100.0 [FLOATING] - 2:9
	0xFF [HEX] - 3:0
	0XAB [HEX] - 3:5
	0o77 [OCTAL] - 4:0
	0O17 [OCTAL] - 4:5
	0b1010 [BINARY] - 5:0
	0B11 [BINARY] - 5:7
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'dot dot is not float' '
	cat >input.orn <<-\EOF &&
	1..10
	EOF
	"$ORN" --dump-tokens input.orn >actual &&
	cat >expect <<-\EOF &&
	1 [NUMBER] - 1:0
	.. [RANGE] - 1:1
	10 [NUMBER] - 1:3
	Program compiled with 0 errors
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
	x [ID] - 1:0
	y [ID] - 2:0
	Program compiled with 0 errors
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
	x [ID] - 1:0
	y [ID] - 2:19
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'empty input produces no tokens' '
	>input.orn &&
	"$ORN" --dump-tokens input.orn >actual &&
	cat > expect <<-\EOF &&
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'unexpected character reports error' '
	echo "@" >input.orn &&
	test_must_fail "$ORN" --dump-tokens input.orn >actual.out 2>actual.err &&
	cat >expect.out <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: unexpected character
	 --> input.orn:1:0
	   |
	 1 | @
	   | ^
	EOF
	test_cmp expect.out actual.out &&
	test_cmp expect.err actual.err
'

test_expect_success 'unterminated string reports error' '
	printf "\"\\n" >input.orn &&
	test_must_fail "$ORN" --dump-tokens input.orn >actual.out 2>actual.err &&
	cat >expect.out <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: unterminated string
	 --> input.orn:1:0
	   |
	 1 | "
	   | ^~
	EOF
	test_cmp expect.out actual.out &&
	test_cmp expect.err actual.err
'

test_expect_success 'unterminated block comment reports error' '
	echo "/* oops" >input.orn &&
	test_must_fail "$ORN" --dump-tokens input.orn >actual.out 2>actual.err &&
	cat >expect.out <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: unterminated block comment
	 --> input.orn:1:0
	   |
	 1 | /* oops
	   | ^~
	EOF
	test_cmp expect.out actual.out &&
	test_cmp expect.err actual.err
'

test_expect_success 'undescore name is gets correctly tokenized' '
	cat >input.orn <<-\EOF &&
	fn _foo() {}
	EOF
	"$ORN" --dump-tokens input.orn >actual &&
	cat >expect <<-\EOF &&
	fn [fn] - 1:0
	_foo [ID] - 1:3
	( [LPAREN] - 1:7
	) [RPAREN] - 1:8
	{ [LBRACE] - 1:10
	} [RBRACE] - 1:11
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_expect_success 'error after number prefix with no digits' '
	echo "0x" >input.orn &&
	test_must_fail "$ORN" --dump-tokens input.orn >actual.out 2>actual.err &&
	cat >expect.out <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: expected digits after the prefix
	 --> input.orn:1:0
	   |
	 1 | 0x
	   | ^~
	EOF
	test_cmp expect.out actual.out &&
	test_cmp expect.err actual.err

'

test_expect_success 'invalid escape sequence in char literal' '
	echo "'\''\\q'\''" >input.orn &&
	test_must_fail "$ORN" --dump-tokens input.orn >actual.out 2>actual.err &&
	cat >expect.out <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: invalid escape sequence
	 --> input.orn:1:0
	   |
	 1 | '\''\q'\''
	   | ^~~~
	EOF
	test_cmp expect.out actual.out &&
	test_cmp expect.err actual.err
'

test_expect_success 'hex escape without digits in char literal' '
	echo "'\''\\x'\''" >input.orn &&
	test_must_fail "$ORN" --dump-tokens input.orn >actual.out 2>actual.err &&
	cat >expect.out <<-\EOF &&
	Program compiled with 1 error
	EOF
	cat >expect.err <<-\EOF &&
	error: expected hex digit after '\''\x'\''
	 --> input.orn:1:0
	   |
	 1 | '\''\x'\''
	   | ^~~~
	EOF
	test_cmp expect.out actual.out &&
	test_cmp expect.err actual.err
'

test_expect_success 'undescore name is gets correctly tokenized' '
	cat >input.orn <<-\EOF &&
	'\''\n'\''
	'\''\t'\''
	'\''\t'\''
	'\''\r'\''
	'\''\\'\''
	'\''\x41'\''
	'\''\0'\''
	EOF
	"$ORN" --dump-tokens input.orn >actual &&
	cat >expect <<-\EOF &&
	'\''\n'\'' [CHAR] - 1:0
	'\''\t'\'' [CHAR] - 2:0
	'\''\t'\'' [CHAR] - 3:0
	'\''\r'\'' [CHAR] - 4:0
	'\''\\'\'' [CHAR] - 5:0
	'\''\x41'\'' [CHAR] - 6:0
	'\''\0'\'' [CHAR] - 7:0
	Program compiled with 0 errors
	EOF
	test_cmp expect actual
'

test_done
