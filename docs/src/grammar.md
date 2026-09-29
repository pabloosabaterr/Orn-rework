# Grammar

This is the grammar of the Orn. Everything in Orn is a range: there are no
built-in types, only sets of integer values written as `lo..hi` (both ends
inclusive). Names like `u8` or `bool` are ordinary constant bindings provided by
the standard library.

```ebnf
program        = { stmt } ;

stmt           = binding ";"
               | "ret" [ expr ] ";"
               | expr ";"
               | block ;

(* The last expression of a block, without ";", is the block's value. *)
block          = "{" { stmt } [ expr ] "}" ;

(*
 *   x :: expr          compile-time constant (aliases, functions)
 *   x := expr          runtime binding, range inferred
 *   x : T = expr       runtime binding, range declared
 *   x : T : expr       compile-time constant, range declared
 *
 * T is an expression evaluated at compile time, so a range or an alias
 * both work: x : 0..255 = 3, x : u8 = 3.
 * Bindings are immutable in the MVP.
 *)
binding        = ID "::" expr
               | ID ":=" expr
               | ID ":" expr ( "=" | ":" ) expr ;

(* A function is an expression; its name comes from the binding. *)
fn_expr        = "(" [ param { "," param } [ "," ] ] ")" [ expr ] block ;
param          = ID ":" expr ;

if_expr        = "if" expr block [ "else" ( if_expr | block ) ] ;

expr           = bound ;

(*
 * Runtime bound: checks the value and narrows it.
 * The else block must leave (ret) or yield a value inside the range.
 *)
bound          = range [ "in" range "else" block ] ;

range          = logic_or [ ".." logic_or ] ;
logic_or       = logic_and { "||" logic_and } ;
logic_and      = equality { "&&" equality } ;

(* Comparisons do not chain: a < b < c is an error. *)
equality       = relational [ ( "==" | "!=" ) relational ] ;
relational     = additive [ ( "<" | ">" | "<=" | ">=" ) additive ] ;

additive       = multiplicative { ( "+" | "-" ) multiplicative } ;
multiplicative = unary { "*" unary } ;

unary          = ( "-" | "!" ) unary
               | postfix ;

postfix        = primary { "(" [ arg_list ] ")" } ;
arg_list       = expr { "," expr } [ "," ] ;

primary        = literal
               | ID
               | "(" expr ")"
               | fn_expr
               | if_expr ;

literal        = NUMBER | HEX | OCTAL | BINARY | "true" | "false" ;
```

## Tokens

- `NUMBER`: decimal integer, `[0-9]+`
- `HEX`: `0x[0-9a-fA-F]+`
- `OCTAL`: `0o[0-7]+`
- `BINARY`: `0b[01]+`
- `ID`: `[a-zA-Z_][a-zA-Z0-9_]*`, except keywords

Keywords: `if`, `else`, `ret`, `in`, `true`, `false`.

Reserved for later: `loop`, `break`, `continue`, `obj`, `enum`, `import`.

## Not in the MVP

Division and modulo (they need the divisor to exclude 0), loops, reassignment,
arrays, open ranges (`..9`), unions of ranges (`0..3 | 10..12`), `distinct`,
floats, strings and pointers.
