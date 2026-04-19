# Types

Every value in Orn has a type that is known at compile time.

## Syntax

Types follow this grammar: (See [Introduction](introduction.md) for the
notation)

```
type = STAR type
     | LBRACKET type SEMI NUMBER RBRACKET
     | LPAREN type_list COMMA? RPAREN
     | base_type

base_type = INT | UINT | FLOAT | DOUBLE
          | BOOL | VOID | CHAR | STRING
          | ID
```

### Primitives

```orn
int       : Signed integer
unsigned  : Unsigned integer
float     : Single precision floating point
double    : Double precision floating point
bool      : Boolean
char      : Character
string    : String
void      : No value
```

`void` is used for functions that do not return a value (omitting the return
type works too). It cannot be used for variables or parameters.

### Pointers

A pointer is the address of a value in memory. A pointer type is written as `*`
followed by the type it points to.

#### Pointer operators:

There are two pointer operators:

- `*T: *T -> T`  : Dereferences a pointer of type `*T` to get the value at the
  address.
- `&T: &T -> *T` : Gets the address of a value of type `T` to create a pointer.

Examples:
```orn
let x: int = 42;
let p: *int = &x; // p points to x, it is an address e.g. 0xABABABAB
let y: int = *p;  // y is now 42
```

### Arrays

An array is a fixed size collection of elements of the same type. The size
has to be known at compile time.

```orn
let arr: [int; 5] = [1, 2, 3, 4, 5];
```

> **want a dynamic array?** See
  [Common Data Structures](common-data-structures.md).

**Arrays and assignment behavior**

Assigng an array to a variable, the variable contains the pointer to the
first element of the array. Slicing creates an indepentent copy of the array.

```orn
let a: [int; 3] = [1, 2, 3];
// A)
let foo = a;
// B)
let foo = a[0:3];
```

- A) `foo` becomes a pointer to the first element of `a`. Modifying `foo`
  modifies `a`.
  What means that it is the same as `let foo = &a[0]`. *Pass by reference*.

- B) `foo` by slicing at the index of the array, it enforces the creation of
  a new array with the sliced elements. Modifying `foo` does not modify `a`.
  *Pass by value*.

This means that `let foo = a[0:n]` is a way to duplicate a whole known size
array.

### Tuples

A tuple groups multiple types together in a fixed order.


```orn
let point: (int, string) = (10, "hello");
```

Tuple fields can be accesed by two ways:

- Destructuring: `let (x, y) = point;`
- Indexing: `point.0` and `point.1`

Tuples work as parameters and return types as well.

### Structs

A struct is a custom type that has named fields.

```orn
struct Point {
	x: int;
	y: int;
}

let p: Point = Point { x: 10, y: 20 };

/* or (to avoid redundancy) */

let p = Point { x: 10, y: 20 };
```
Omitting the type annotation when the right side is a struct initializer is
good practice to avoid noise and keep the code clean.

### Enums

An enum is a type with fixed set of members. Each member can carry data
(tagged unions) or have explicit integer values.

```orn
enum Color {
	Red = 1,
	Green,
	Blue,
	RGB(int, int, int)
}
```
There are multiple ways to access enum members:
```orn
let c: Color = Color::Red;  // full declaration
let c: Color = ::Red;       // object inferred from static type
let c = Color::Red;         // type inferred from right side
```
There is this exception where the object can be 'omited', these cases are when
a function returns an object or we are matching on an enum:

```orn
fn get_color() -> Color {
	ret ::Red; // the object is inferred from the return type
}
```

```orn
match (color) {
	::Red => { ... } // the object is inferred from the matched value
	::Green => { ... }
	_ => { ... }
}
```

More about enums behavior in [statements](NEEDSWORK.md).

### Impl

Not actually a type, but worth mentioning it here.

Both structs and enums can have functions along with them using `impl` blocks.
This functions are static and accessed with the `::` operator.

```orn
struct Point {
	x: int;
	y: int;
}

impl Point {
	fn origin() -> Point {
		ret Point { x: 0, y: 0 };
	}

	fn translate(p: *Point, dx: int, dy: int) {
		p.x += dx;
		p.y += dy;
	}
}

let p = Point::origin();
Point::translate(&p, 5, 10);
```

### Type aliases

A type alias adds alternative names for exiting types. The alias and the
original are interchangeable; type aliases are a compile time feature, not a
new type.

```
type_dec = TYPE ID EQ type SEMI
```

```orn
type Byte = unsigned;
type Pair = (int, int);

let p: Pair = (10, 20);
```

## Casting

Casting is done with the `as` operator:
```orn
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
