# Types

Every value in Orn has a type that it´s known at compile time.

## Syntax

```
type = STAR type
     | LBRACKET type SEMI NUMBER RBRACKET
     | LPAREN type_list COMMA? RPAREN
     | base_type

type_list = type (COMMA type)*

base_type = INT
          | UINT
          | FLOAT
          | DOUBLE
          | BOOL
          | VOID
          | CHAR
          | STRING
          | ID
```

## Primitives

| Type | Description |
|-|-|
| `int` | Signed integer |
| `uint` | Unsigned integer current keyword is `unsigned` |
| `float` | Single precision floating point |
| `double` | Double precision floating point |
| `bool` | Boolean |
| `char` | Character |
| `string` | String |
| `void` | No value |

`void` is used for functions that do not return a value (omitting the return
type works too). It cannot be used for variables or parameters.

## Pointers

A pointer type is writtes as `*` followed by the type it points to.

```
pointer_type = STAR type
```

`&` works to take the address of a value and `*` to dereference it.

## Array types

```
array_type = LBRACKET type SEMI NUMBER RBRACKET
```
e.g:
```
let arr: [int; 5] = [1, 2, 3, 4, 5];
```

**Arrays and assignment behavior**

```
let a: [int; 3] = [1, 2, 3];
A) let foo = a;
B) let foo = a[0:3];
```

- A) `foo` becomes a pointer to the first element of `a`. Modifying `foo`
  modifies `a`.
  What means that it is the same as `let foo = &a[0]`. *Pass by reference*.

- B) `foo` by slicing at the index of the array, it enforces the creation of
  a new array with the sliced elements. Modifying `foo` does not modify `a`.
  *Pass by value*.

This means that `let foo = a[0:n]` is a way to duplicate a whole known size
array.

## Tuples

A tuple groups multiple types together in a fixed order.

```
tuple_type = LPAREN type_list COMMA? RPAREN
```
e.g:
```
let point: (int, string) = (10, "hello");
```

Tuple fields can be accesed by two ways:

- Destructuring: `let (x, y) = point;`
- Indexing: `point.0` and `point.1`

Tuples work as parameters and return types.

## Named types

Any identifier can appear where a type is expected.

```
struct Point {
	x: int;
	y: int;
}

let p: Point = Point { x: 10, y: 20 };

or (to avoid redundancy)

let p = Point { x: 10, y: 20 };
```
This is good practice to avoid noise and keep the code clean.

```
enum Color {
	Red,
	Green,
	Blue,
}
```
There are multiple ways to access enum members:
```
let c: Color = ::Red;
let c: Color = Color::Red;
let c = Color::Red;
```
It is not accepted
```
let c = ::Red;
```
Orn aims to be light but preserve as much expressiveness as possible.

## Type aliases

```
type_dec = TYPE ID EQ type SEMI
```

```
type Byte = unsigned;
type Pair = (int, int);

let p: Pair = (10, 20);
```
The alias and the original type are interchangeable. This is a compile time
feature to improve readability and expressiveness it is not a new type.

## Casting

Casting is done with the `as` operator:
```
let x: int = 65;
let ch: char = x as char;     // 'A'
let f: float = x as float;    // 65.0
```

## Where types appear

Types appears in several parts of the code and they are a fundamental part of
the language. They are used at:

- Variable declarations: `let x: int = 42;`
- Constants: `const PI: double = 3.14159;`
- Function parameters and return types: `fn add(a: int, b: int) -> int {...}`
- Struct fields: `struct Point { x: int; y: int; }` (Struct is a type itself)
- Enum declarations: `enum Color { Red, Green, Blue }` (Enum is a type itself)
- Type aliases: `type Byte = unsigned;`
- Casts: `let ch: char = x as char;`
- sizeof operator: `let size = sizeof(int);`

When a let declaration does not specify a type, it adopts the type from the
right side of the assignment. This is a type of inference that allows to omit
many redundant type annotations.
