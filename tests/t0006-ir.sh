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

test_done
