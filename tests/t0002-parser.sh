#!/bin/sh

# shellcheck disable=SC1091,SC2016,SC2034
TDIR=$(pwd)
. "$(dirname "$0")/test-lib.sh"
setup_test_dir "$(basename "$0")" || exit 1

EXAMPLES="$TDIR/../examples"

for f in "$EXAMPLES"/*.orn
do
	test_expect_success "example compiles: $(basename "$f")" "
		orn \"$f\" >/dev/null 2>&1
	"
done

# Until the range pass exists these compile, so they are known failures.
for f in "$EXAMPLES"/errors/*.orn
do
	test_expect_failure "example is rejected: errors/$(basename "$f")" "
		test_must_fail orn \"$f\" >/dev/null 2>&1
	"
done

test_done
