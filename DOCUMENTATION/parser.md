# Orn Language Grammar : v1

## Notation

```
x y     : sequence
x | y   : alternative
x?      : optional
x*      : zero or more
x+      : one or more
"x"     : literal token
UPPER   : token from lexer
(x y)   : grouping
```

## Tokens (Lexer)

```
NUMBER = DECIMAL | HEX_LITERAL | OCT_LITERAL | BIN_LITERAL

DECIMAL = [0-9]+ ("." [0-9]+)?
HEX_LITERAL = "0x" [0-9a-fA-F]+
OCT_LITERAL = "0o" [0-7]+
BIN_LITERAL = "0b" [01]+

STRING_LITERAL = "\"" (escape | [^"\\])* "\""
CHAR_LITERAL = "'" (escape | [^'\\]) "'"

escape = "\\" [nrt0\\'"x]
       | "\\x" [0-9a-fA-F]{2}

bool_literals: TRUE | FALSE

ID = [a-zA-Z_] [a-zA-Z0-9_]*
```

## Program

```
program = dec*
```

## Declarations

```
dec = fn_dec | struct_dec | impl_dec | enum_dec | type_dec
    | let_dec | const_dec | import_dec

fn_dec = FN ID LPAREN param_list? RPAREN (RARROW type)? block

struct_dec = STRUCT ID LBRACE field_list+ RBRACE

impl_dec = IMPL ID LBRACE fn_dec* RBRACE

enum_dec = ENUM ID LBRACE enum_list RBRACE

enum_list = enum_member (COMMA enum_member)* COMMA?

enum_member = ID (LPAREN type_list RPAREN)? (EQ expr)?

type_dec = TYPE ID EQ type SEMI

let_dec = LET (ID | LPAREN id_list RPAREN) (COLON type EQ expr | COLON type
        | EQ expr) SEMI

const_dec = CONST ID COLON type EQ expr SEMI

import_dec = IMPORT STRING_LITERAL SEMI

param_list = param (COMMA param)* (COMMA SPREAD)?

param = ID COLON type

field_list = ID COLON type SEMI

id_list = ID (COMMA ID)*
```

## Statements

```
stmt = block | if_stmt | while_stmt | for_stmt | return_stmt
     | break_stmt | continue_stmt | defer_stmt | match_stmt
     | incdec_stmt | expr_stmt

block = LBRACE stmt* RBRACE

if_stmt = IF expr stmt (ELIF expr stmt)* (ELSE stmt)?

while_stmt = WHILE expr stmt

for_stmt = FOR ID IN range_expr stmt

return_stmt = RETURN expr? SEMI

break_stmt = BREAK SEMI

continue_stmt = CONTINUE SEMI

defer_stmt = DEFER stmt

match_stmt = MATCH LPAREN expr RPAREN LBRACE match_arm+ RBRACE

match_arm = match_pattern FATARROW stmt

match_pattern = ID (LPAREN id_list RPAREN)?
              | expr
              | UNDERSCORE

incdec_stmt = expr (INCREMENT | DECREMENT) SEMI

expr_stmt = expr SEMI
```

## Expressions

```
expr = assign_expr

assign_expr = unary_expr
              (EQ | PLUSEQ | MINUSEQ | STAREQ | SLASHEQ | MODEQ) assign_expr
            | range_expr

range_expr = or_expr (RANGE or_expr)?

or_expr = and_expr (OR and_expr)*

and_expr = bitor_expr (AND bitor_expr)*

bitor_expr = bitxor_expr (PIPE bitxor_expr)*

bitxor_expr = bitand_expr (CARET bitand_expr)*

bitand_expr = eq_expr (AMP eq_expr)*

eq_expr = cmp_expr ((CMP | NEQ) cmp_expr)*

cmp_expr = shift_expr ((LT | GT | LE | GE) shift_expr)*

shift_expr = add_expr ((LSHIFT | RSHIFT) add_expr)*

add_expr = mul_expr ((PLUS | MINUS) mul_expr)*

mul_expr = cast_expr ((STAR | SLASH | MOD) cast_expr)*

cast_expr = unary_expr (AS type)*

unary_expr = (MINUS | NOT | TILDE | AMP | STAR) unary_expr
           | postfix_expr

postfix_expr = primary_expr
             (call_args | index_access | member_access | ns_access)*

call_args = LPAREN arg_list? RPAREN

index_access = LBRACKET expr (COLON expr?)? RBRACKET

member_access = DOT (ID | NUMBER)

ns_access = NAMESPACE ID

arg_list = expr (COMMA expr)*

field_init_list = field_init (COMMA field_init)* COMMA?

field_init = ID COLON expr

primary_expr = ID
             | NUMBER | STRING_LITERAL | CHAR_LITERAL | TRUE | FALSE
             | LPAREN expr RPAREN
             | SIZEOF LPAREN type RPAREN
             | SYSCALL LPAREN arg_list? RPAREN
             | ID LBRACE field_init_list RBRACE
             | LBRACKET arg_list? RBRACKET
	     | NAMESPACE ID
             | NULL
```

## Types

```
type = STAR type
     | LBRACKET type SEMI NUMBER RBRACKET
     | LPAREN type_list COMMA? RPAREN
     | base_type

type_list = type (COMMA type)*

base_type = INT | UNSIGNED | FLOAT | DOUBLE | BOOL
          | VOID | CHAR | STRING | ID
```

## Operators (precedence low to high)

```
1.  assignment      =  +=  -=  *=  /=  %=        (right associative)
2.  range           ..                           (left)
3.  logical or      ||                           (left)
4.  logical and     &&                           (left)
5.  bitwise or      |                            (left)
6.  bitwise xor     ^                            (left)
7.  bitwise and     &                            (left)
8.  equality        ==  !=                       (left)
9.  comparison      <  >  <=  >=                 (left)
10. shift           <<  >>                       (left)
11. additive        +  -                         (left)
12. multiplicative  *  /  %                      (left)
13. cast            as                           (left)
14. unary prefix    -  !  ~  &  *                (right)
15. postfix         ()  []  .  ::                (left)
```

Note: `++` and `--` are not operators. They are statement-level only
(see `incdec_stmt`). You cannot use them inside expressions.

## Namespace vs Member Access

```
::  (namespace access)  : resolves a static function inside an impl block.
                          No implicit receiver. Called as: Vec::new()

.   (member access)     : accesses a field on a value. In v1.2 when self
                          is introduced, it will also dispatch methods.
                          Examples: point.x, point.distance(other)
```

In v1, `impl` groups static functions under a struct's namespace:

```
struct Vec { data: *int; len: int; }

impl Vec {
    fn new() -> Vec { ... }
    fn push(v: *Vec, val: int) { ... }
}

usage:
let v = Vec::new();
Vec::push(&v, 42);
```

## Enums with associated data (tagged unions)

Orn enums support associated data from v1. Variants without data
need no parentheses. Variants with data specify the type:

```
enum Option {
    Some(int),
    None,
}

enum Result {
    Ok(int),
    Err(string),
}

enum Token {
    Number(int),
    Ident(string),
    Eof,
}
```

Match destructures variants:

```
match (value) {
    Some(n) => { print(n); }
    None => { print("nothing"); }
}

match (result) {
    Ok(n) => { print(n); }
    Err(msg) => { print(msg); }
    _ => {}
}
```

Plain enums without data work like classic C enums:

```
enum Color {
    Red,
    Green,
    Blue,
}
```

Explicit values are allowed when variants carry no data:

```
enum Flags {
    Read = 1,
    Write = 2,
    Exec = 4,
}
```

## For loops

There is one form of for. Range-based, no parentheses:

```
for i in 0..10 {
    print(i);
}
```

For arbitrary conditions or custom step, use `while`:

```
let i = 0;
while i < 100 {
    ...
    i += 3;
}
```

## Increment / Decrement

`++` and `--` exist but they are statements, not expressions.
Same approach as Go. You can write `i++;` on its own line but
you cannot nest it inside an expression like `a = i++;`.

```
let i = 0;
i++;         ok, standalone statement
i--;         ok

a = i++;  compile error: ++ is not an expression
f(i++);   compile error: same reason
```

This keeps the familiar syntax without the ambiguity problems
that come from mixing side effects with expression evaluation.

## Design Notes

**No parentheses in conditions**: `if`, `while`, `elif` and `for`
do not require parentheses. The body is always a block `{ }`, so
the parser knows where the condition ends without ambiguity.

**let requires type or initializer**: `let x;` is a compile error.
Write `let x: int;`, `let x = 0;`, or `let x: int = 0;`.

**++ and -- are statements**: they cannot appear inside expressions.
`i++;` is valid. `a = i++;` is not. Keeps things simple in the parser
and kills a whole class of bugs around evaluation order.

**impl as namespace (v1)**: without `self`, impl blocks are just
namespaces for static functions. Access via `::`. When `self` lands
in v1.2, `::` stays for static calls and `.` becomes method sugar.

**defer**: runs a statement when the enclosing scope exits. Useful
for closing files, freeing memory, etc.

**syscall**: built-in keyword for system calls. Orn is a systems
language with direct OS control.
