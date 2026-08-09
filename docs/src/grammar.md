```
program    = stmt*

stmt       = binding SEMI
           | label
           | GOTO label SEMI
           | loop_stmt
           | RET expr? SEMI
           | BREAK SEMI
           | CONTINUE SEMI
           | expr SEMI

label      = HASH ID

block      = LBRACE stmt* RBRACE

loop_stmt  = LOOP (ID WALRUS expr_nb | expr_nb)? block

binding    = ID DECL expr
           | ID WALRUS expr
           | ID COLON expr (DECL expr | EQ expr)?

fn_expr    = params expr? block?
params     = LPAREN (param (COMMA param)* COMMA?)? RPAREN
param      = ID COLON SPREAD? expr

member     = DOT? binding SEMI
obj_lit    = OBJ LBRACE member* RBRACE
enum_lit   = ENUM LBRACE (variant (COMMA variant)* COMMA?)? member* RBRACE
variant    = ID (EQ expr)?

if_expr    = IF expr_nb block (ELSE (if_expr | block))?

expr       = assign
assign     = range (assign_op assign)?
assign_op  = EQ | PLUSEQ | MINUSEQ | STAREQ | SLASHEQ | MODEQ
range      = logic_or (RANGE logic_or)?
logic_or   = logic_and (OR logic_and)*
logic_and  = equality (AND equality)*
equality   = relational ((CMP | NEQ) relational)*
relational = bit_or ((LT | GT | LE | GE) bit_or)*
bit_or     = bit_xor (PIPE bit_xor)*
bit_xor    = bit_and (CARET bit_and)*
bit_and    = shift (AMP shift)*
shift      = additive ((LSHIFT | RSHIFT) additive)*
additive   = multiplicative ((PLUS | MINUS) multiplicative)*
multiplicative = unary ((STAR | SLASH | MOD) unary)*

unary      = (MINUS | NOT | TILDE | STAR | AMP) unary
           | LPAREN expr RPAREN unary
           | LBRACKET expr RBRACKET unary
           | postfix

postfix    = primary suffix*
suffix     = DOT ID
           | DOT NUMBER
           | LPAREN arg_list? RPAREN
           | LBRACKET expr RBRACKET
           | LBRACKET expr? COLON expr? RBRACKET

primary    = literal
           | ID
           | primitive
           | UNDERSCORE
           | DOT ID
           | LPAREN expr_list? RPAREN
           | LBRACKET expr_list? RBRACKET
           | fn_expr
           | obj_lit
           | enum_lit
           | if_expr
           | IMPORT STRINGLIT

literal    = NUMBER | FLOATING | STRINGLIT | CHARLIT
           | TRUE | FALSE | NULL

primitive  = IN | UN | FL | DB | BL | CH | ST | VD | TP

arg_list   = arg (COMMA arg)* COMMA?
arg        = (ID COLON)? expr
expr_list  = expr (COMMA expr)* COMMA?
```
