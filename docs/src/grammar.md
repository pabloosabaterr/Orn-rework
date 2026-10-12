# Grammar

This is the grammar of the Orn MVP. Everything in Orn is a range: there are no
built-in types, only sets of integer values written as `lo..hi` (both ends
inclusive). Names like `u8` or `bool` are ordinary constant bindings.

```ebnf
program        = { stmt } ;

stmt           = ( binding | return | if_stmt | loop_stmt | block | expr ) ";" ;
block          = "{" { stmt } "}" ;
return         = "ret" [ expr ] ;
if_stmt        = "if" expr block [ "else" ( if_stmt | block ) ] ;
loop_stmt      = "loop" ( ID "in" expr | expr ) block ;

binding        = ID "::" expr
               | ID ":=" expr
               | ID ":" expr ( "=" | ":" ) expr ;

expr           = expr binop expr
               | prefix_op expr
               | expr "(" [ args ] ")"
               | primary ;
binop          = "||" | "&&"
               | "==" | "!=" | "<" | ">" | "<=" | ">="
               | ".." | "+" | "-" | "*" | "/" | "%" ;
prefix_op      = "-" | "!" ;
args           = expr { "," expr } ;

primary        = literal | ID | "(" expr ")" | fn_expr ;
fn_expr        = "(" [ params ] ")" [ expr ] block ;
params         = param { "," param } ;
param          = ID ":" expr ;

literal        = NUMBER | HEX | OCTAL | BINARY | "true" | "false" ;
```

## Statements

Every statement ends in `;`, including `if` and blocks. Blocks have no value:
values leave a function only through `ret`.

## Loops

`loop i in a..b { ... }` runs the block once for each value of the range, in
order. Inside the block, `i` has the range of the loop:

```
loop i in 0..9 {
    // i : 0..9
};
```

The range is any expression, so an alias works too: `loop i in u8 { ... }`.

`loop cond { ... }` runs the block while `cond` holds. The parser tells both
forms apart by looking past `loop`: `ID in` starts a range loop, anything else
is a condition.

## Bindings

| Form          | Meaning                                   |
|---------------|-------------------------------------------|
| `x :: e`      | compile-time constant (aliases, functions) |
| `x := e`      | runtime binding, range inferred           |
| `x : T = e`   | runtime binding, range declared           |
| `x : T : e`   | compile-time constant, range declared     |

`T` is an expression evaluated at compile time, so a range or an alias both
work: `x : 0..255 = 3`, `x : u8 = 3`. Bindings are immutable in the MVP.

## Functions

A function is an expression; its name comes from the binding it is assigned
to. If it declares a return range, every path must end in `ret`.

`fn_expr` and a parenthesized expression both start with `(`. The parser
tells them apart by looking past it: `()` or `( ID :` starts a function,
anything else is grouping.

## Expressions

The `expr` rule is ambiguous on purpose: the parser is a Pratt parser, and
how operators group comes from this table, from lowest to highest precedence:

| Operators                   | Associativity |
|-----------------------------|---------------|
| `\|\|`                      | left          |
| `&&`                        | left          |
| `==` `!=` `<` `>` `<=` `>=` | none          |
| `..`                        | none          |
| `+` `-`                     | left          |
| `*` `/` `%`                 | left          |
| `-` `!` (prefix)            | right         |
| call `f(x)`                 | left          |

Non-associative operators do not chain: `a < b < c` and `a..b..c` are errors.
Since ranges bind tighter than comparisons, `x < 0..10` is `x < (0..10)`, and
since they bind looser than arithmetic, `0..n + 1` is `0..(n + 1)`.

In `if` and `loop`, the expression ends where the block starts: `{` cannot
continue an expression.

## Comparisons narrow ranges

Inside each branch of an `if`, the comparisons in the condition narrow the
ranges of the values they test:

```
u8 :: 0..255;

classify :: (i: 0..1000) 0..2 {
    if i < 10 {
        ret 0;      // i : 0..9
    } else if i < 100 {
        ret 1;      // i : 10..99
    } else {
        ret 2;      // i : 100..1000
    };
};
```

## Tokens

- `NUMBER`: decimal integer, `[0-9]+`
- `HEX`: `0x[0-9a-fA-F]+`
- `OCTAL`: `0o[0-7]+`
- `BINARY`: `0b[01]+`
- `ID`: `[a-zA-Z_][a-zA-Z0-9_]*`, except keywords

Keywords: `if`, `else`, `loop`, `in`, `ret`, `true`, `false`.

Reserved for later: `break`, `continue`, `obj`, `enum`, `import`.

## Not in the MVP

Block values and `if` as an expression, runtime bounds (`x in 0..9 else { … }`),
reassignment, arrays, open ranges (`..9`), unions of ranges (`0..3 | 10..12`), `distinct`,
floats, strings and pointers.
