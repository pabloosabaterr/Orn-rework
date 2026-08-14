#!/bin/sh

ORN_BUILD_DIR=$(cd ../build && pwd)
PATH="$ORN_BUILD_DIR:$PATH"
export PATH

RED=$(printf '\033[31m')
GREEN=$(printf '\033[32m')
YELLOW=$(printf '\033[33m')
BLUE=$(printf '\033[34m')
BOLD=$(printf '\033[1m')
RESET=$(printf '\033[0m')

test_count=0
test_failed=0
known_bugs=0

test_cmp () {
	diff -u "$1" "$2"
}

test_must_fail () {
	"$@"
	status=$?
	if [ "$status" -eq 0 ]
	then
		echo "test_must_fail: command succeeded unexpectedly: $*" >&2
		return 1
	elif [ "$status" -gt 128 ]
	then
		echo "test_must_fail: died by signal $((status - 128)): $*" >&2
		return 1
	fi
	return 0
}

test_expect_success () {
	test_count=$((test_count + 1))
	if eval "$2"
	then
		echo "ok $test_count - $1"
	else
		test_failed=$((test_failed + 1))
		echo "${RED}not ok $test_count - $1$RESET"
	fi
}

test_expect_failure () {
	test_count=$((test_count + 1))
	eval "$2"
    status=$?
    if [ "$status" -eq 0 ]
	then
		echo "ok $test_count - $1 #BUG fixed, change _expect_failure"
	else
        known_bugs=$((known_bugs + 1))
		echo "not ok $test_count - $1 $YELLOW#BUG known failure$RESET"
	fi
}

test_done () {
    cd "$TDIR" && rm -rf "$TRASH"
    echo
	printf "passed: %d/%d\nfailed: %d\nknown bugs: %d" \
        "$((test_count - test_failed))" \
        "$test_count" \
        "$test_failed" \
        "$known_bugs"
	test "$test_failed" = 0
	exit $?
}

setup_test_dir() {
	echo "- $1:"
    name="${1%.sh}"
    TRASH="$TDIR/trash-$name"
    rm -rf "$TRASH" || return 1
    mkdir -p "$TRASH" || return 1
    cd "$TRASH" || return 1
    export GIT_CEILING_DIRECTORIES="$TRASH"
}
