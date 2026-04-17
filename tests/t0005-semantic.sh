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

test_done
