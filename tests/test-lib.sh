#!/bin/sh

test_count=0
pass_count=0
fail_count=0

TEST_DIR_ROOT=$(cd "$(dirname "$0")/.." && pwd)
ORN="$TEST_DIR_ROOT/build/orn"

test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
cd "$test_dir" || exit 1

test_expect_success () {
	test_count=$((test_count + 1))
	desc="$1"
	shift
	if eval "$*"
	then
		pass_count=$((pass_count + 1))
		echo "ok $test_count - $desc"
	else
		fail_count=$((fail_count + 1))
		echo "not ok $test_count - $desc"
	fi
}

# Reports known failures as successes, so that the Bugs are being tracked.
test_expect_failure () {
	test_count=$((test_count + 1))
	desc="$1"
	shift
	if ! eval "$*"
	then
		pass_count=$((pass_count + 1))
		echo "not ok $test_count - $desc # TODO known failure"
	else
		fail_count=$((fail_count + 1))
		echo "ok $test_count - $desc # TODO fixed, change to test_expect_success"
	fi
}

# Actually spects it to fail, and reports it as such if it doesn't.
test_must_fail () {
	"$@"
	exit_code=$?
	if test $exit_code -eq 0
	then
		return 1
	elif test $exit_code -gt 128 && test $exit_code -le 192
	then
		echo >&2 "test_must_fail: died by: $*"
		return 1
	fi
	return 0
}

test_cmp () {
	diff -u "$1" "$2"
}

test_must_be_empty () {
	test ! -s "$1"
}

test_done () {
	echo "1..$test_count"
	if test "$fail_count" -gt 0
	then
		echo "# $fail_count test(s) failed"
		exit 1
	fi
	exit 0
}
