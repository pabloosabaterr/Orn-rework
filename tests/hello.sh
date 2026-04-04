#!/bin/sh

out=$(./build/orn)
expected="Welcome to Orn!"

if test "$out" = "$expected"; then
	echo "ok 1 - program prints '$expected'"
else
	echo "not ok 1 - expected '$expected', got '$out'"
	exit 1
fi

echo "1..1"
