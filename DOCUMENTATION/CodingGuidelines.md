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