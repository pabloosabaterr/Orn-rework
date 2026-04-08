# Lexer

The lexer is the first stage of the compiler. Takes the input source code and
produces the tokens that the parser will consume.

## Design

The Lexer is a single pass scanner. It does not allocate the tokens but rather
it sends them one by one as the caller requests them.

It has a state where the current position, line and column are tracked.
The caller has the responsibility to init the context and to free it. With the
initialized context, the caller can call for the `token_next()` to get the next
token, until the `TK_EOF` is reached.

## Tokens

```c
struct token {
	enum token_type type;
	const char *lex;
	size_t len;
	int line;
	int col;
};
```

A token carries its type, the pointer and length of its lexeme and the location
of it. Tokens are not null terminated, so when comparing or printing them, it is
important to use the length.

## Token categories

### Literals

Number, string and char literals. String and char literals include their quotes
in the token text. Escape sequences are recognized so the lexer does not
terminate early, but they are not interpreted. That is the job of a later stage.

### Keywords

Keywords are identified by scanning an identifier and checking it against a
lookup table. If it matches, the token gets the keyword type instead of `TK_ID`.

### Operators and punctuation

Single character tokens are dispatched from a switch. Multi character operators
are resolved by checking the next character with `follow_next`.

### Special tokens

- `TK_EOF` marks the end of input.
- `TK_ERROR` marks a lexical error. The message is printed to stderr when
  created and the token text contains the offending characters.

## Whitespace and comments

Whitespace and comments are skipped between tokens. They do not produce tokens.
Comments follow C style: `//` for line and `/* */` for block.
An unterminated block comment is a fatal error.

## Error handling

Lexical errors produce `TK_ERROR` tokens and print a diagnostic to stderr. The
lexer keeps scanning after an error so that multiple errors can be reported in
one pass.

Unterminated block comments call `die()` because the lexer cannot continue.

## Contracts for the parser

1. Every call to `token_next` returns a valid token.
2. The stream always ends with `TK_EOF`.
3. `TK_EOF` is returned repeatedly once the end is reached.
4. Token pointers are valid for the lifetime of the source buffer.
5. Keywords are resolved at the lexer level. Identifiers like variable names
   come as `TK_ID`, keywords like `let` or `int` come as their own type.
6. String and char literals include their delimiters.
