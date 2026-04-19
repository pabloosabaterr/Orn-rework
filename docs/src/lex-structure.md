# Keywords

Orn has some reserved keywords that they cannot be used as identifiers or
outside of their proper contexts. These are:

- `int`
- `unsigned`
- `double`
- `float`
- `bool`
- `void`
- `char`
- `string`
- `if`
- `else`
- `elif`
- `while`
- `ret`
- `break`
- `continue`
- `struct`
- `enum`
- `import`
- `true`
- `false`
- `let`
- `const`
- `fn`
- `sizeof`
- `syscall`
- `for`
- `in`
- `match`
- `defer`
- `impl`
- `type`
- `as`
- `null`

## Identifiers

```
IDENTIFIER = [a-zA-Z_] [a-zA-Z0-9_]*
```
Identifiers that are are valid:

- `foo`
- `_foo`
- `foo123`
- `foo_bar`
- `FOO`

Identifiers that start with an underscore are usually used for intentionally
unused variables, for example:

```orn
fn foo() {
	let _unused = 42;
}
```

This won't trigger `unused variable` warning.

## Literals

```
NUMBER = DECIMAL | HEX_LITERAL | OCT_LITERAL | BIN_LITERAL

DECIMAL = [0-9]+ ("." [0-9]+)?
HEX_LITERAL = "0" ("x" | "X") [0-9a-fA-F]+
OCT_LITERAL = "0" ("o" | "O") [0-7]+
BIN_LITERAL = "0" ("b" | "B") [01]+

STRING_LITERAL = "\"" (escape | [^"\\])* "\""
CHAR_LITERAL = "'" (escape | [^'\\]) "'"

escape = "\\" [nrt0\\'"x] | "\\x" [0-9a-fA-F]{2}

bool_literals = TRUE | FALSE
```

Number literals:
- `42`
- `3.14`
- `0x2A`
- `0o52`
- `0b101010`

Hex, octal and binary becomes decimal numbers at compile time at the parser.

String literals:
- `"Hello, world!"`
- `"Line 1\nLine 2"`

Character literals:
- `'a'`
- `'\n'`

Bool literals:
- `true`
- `false`

## Operators and Punctuation

### Punctuation

Punctuation tokens are used as operators, separators, and delimiters.

```
PUNCTUATION = "..."
            | "+="
            | "-="
            | "*="
            | "/="
            | "%="
            | "=="
            | "!="
            | "<="
            | ">="
            | "<<"
            | ">>"
            | "&&"
            | "||"
            | "->"
            | "<-"
            | "=>"
            | "::"
            | ".."
            | "++"
            | "--"
            | "+"
            | "-"
            | "*"
            | "/"
            | "%"
            | "!"
            | "~"
            | "&"
            | "|"
            | "^"
            | "="
            | "<"
            | ">"
            | "."
            | ","
            | ":"
            | ";"
            | "?"
            | "_"
            | "("
            | ")"
            | "{"
            | "}"
            | "["
            | "]"
```

Multi character tokens are matched longest first.

### Delimiters

Bracket punctuation must always appear in matched pairs. The three
types are:

| Bracket | Name           |
|---------|----------------|
| `{ }`   | braces         |
| `[ ]`   |  brackets      |
| `( )`   | parentheses    |

An unmatched delimiter is a compile error.
