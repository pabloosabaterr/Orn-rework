# Coding Style

This project follows the coding conventions of the
[Git project](https://github.com/git/git/blob/master/Documentation/CodingGuidelines).
The `.clang-format` file in the repository root enforces most of these rules
automatically.

## C

- Indent with tabs, 8 characters wide.
- Opening brace on the same line as the statement, except for functions:

```c
void foo(void)
{
	if (condition)
		do_something();
	else
		do_other();
}
```

  If any branch in an if/else chain needs braces, all of them get braces:

```c
if (condition) {
	do_something();
	do_more();
} else {
	do_other();
}
```

- Pointer declarations bind to the name, not the type: `int *ptr`.
- Aim for 80 characters per line. Exceeding it is fine when it helps
  readability.
- No trailing whitespace.
- One blank line between functions, never more than one consecutive blank
  line within a function.
- `#include` order is not sorted automatically — group them logically
  and keep related headers together.

## Shell

- Indent with tabs, 8 characters wide.
- Use `$(...)` instead of backticks.
- Redirections without space after the operator: `echo hello >file`.
- Use `type` instead of `which` to check for commands.

### Test structure

Test files live in `tests/` and follow the naming pattern
`txxxx-<topic>.sh`. Each file sources the test framework and defines
individual test cases:

```sh
#!/bin/sh

# shellcheck disable=SC1091,SC2016,SC2034
. "$(dirname "$0")/test-lib.sh"

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

test_done
```

Mark tests for known failures with `test_expect_failure` and add a
`NEEDSWORK` comment in the relevant source file. e.g:

```sh
test_expect_failure 'invalid escape sequence should be rejected' '
	cat >input.orn <<-\EOF &&
	fn foo() {
		let s: string = "\q";
	}
	EOF
	test_must_fail "$ORN" input.orn 2>/dev/null
'
```

## Commits

- Subject line: max 72 characters, imperative mood ("Add feature" not
  "Added feature").
- Blank line between subject and body.
- Wrap the body at 72 characters.
- Explain *why*, not just *what*.

## Formatting

Run `make format` before committing, or configure your editor to run
`clang-format` on save. To verify without modifying files:

```
make check-format
```
