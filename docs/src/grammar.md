# Grammar

This is the grammar of the Orn MVP. Everything in Orn is a range: there are no
built-in types, only sets of integer values written as `lo..hi` (both ends
inclusive). Names like `u8` or `bool` are ordinary constant bindings.

```ebnf
program        = { stmt } ;

(* Every statement ends in ";", including if and blocks. *)
stmt           = ( binding
                 | "ret" [ expr ]
                 | if_stmt
                 | block
                 | expr ) ";" ;

(* Blocks have no value: values leave a function through ret. *)
block          = "{" { stmt } "}" ;

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

(*
 * A function is an expression; its name comes from the binding.
 * If it declares a return range, every path must end in ret.
 *)
fn_expr        = "(" [ param { "," param } [ "," ] ] ")" [ expr ] block ;
param          = ID ":" expr ;

(* Comparisons in the condition narrow ranges inside each branch. *)
if_stmt        = "if" expr block [ "else" ( if_stmt | block ) ] ;

expr           = range ;

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
               | fn_expr ;

literal        = NUMBER | HEX | OCTAL | BINARY | "true" | "false" ;
```

Example:

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

Keywords: `if`, `else`, `ret`, `in`, `true`, `false`.

Reserved for later: `loop`, `break`, `continue`, `obj`, `enum`, `import`.

## Not in the MVP

Block values and `if` as an expression, runtime bounds (`x in 0..9 else { … }`),
division and modulo (they need the divisor to exclude 0), loops, reassignment,
arrays, open ranges (`..9`), unions of ranges (`0..3 | 10..12`), `distinct`,
floats, strings and pointers.
