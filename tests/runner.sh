# !/bin/sh

total_passed=0
total_fail=0
known_bugs=0

for t in t[0-9]*.sh
do
    output=$(sh "$t")
    status=$?

    printf "%s\n\n" "$output"

    line=$(printf "%s" "$output" | grep '^passed:')

    p=$(echo "$line" | awk '{print $2}' | cut -d/ -f1)
    f=$(echo "$line" | awk '{print $4}')
    b=$(echo "$line" | awk '{print $7}')

    passed=$((passed + p))
    failed=$((failed + f))
    bugs=$((bugs + b))

    [ $status -eq 0 ] || true
done

echo "======================================"
echo
echo "TOTAL:\npassed = $passed\nfailed = $failed\nknown bugs = $bugs"
if [ $failed -eq 0 ]
then
    echo "\033[32mTESTS PASSED CORRECTLY\033[0m"
else
    echo "\033[31mTESTS FAILED\033[0m"
fi
echo

test "$failed" -eq 0
