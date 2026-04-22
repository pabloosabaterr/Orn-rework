# Program structure

## Top-level code

Orn does not require a `main` function. Code can be written at the top level
and it will execute from top to bottom:

```orn
let x = 10;
let y = 20;
let sum = x + y;
```

> Using a `main` function is recommended for clarity, but not required.

## Top hoisting

Top-level signatures (functions, structs, enums, type aliases, global variables)
are hoisted, so they can be used before they appear in the file. Top-level
statements execute in order.

```orn
let p = Point::origin(); // works, Point is declared below

struct Point { x: int; y: int; }

impl Point {
	fn origin() -> Point {
		ret Point { x: 0, y: 0 };
	}
}
```

This means that the order of signatures does not matter, but the order
of statements does.

For more information about the semantics of Orn, see
[Semantic analysis](semantic-pass.md).
