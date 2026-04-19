# Roadmap

The roadmap for Orn is what is expected to keep developing after v1.0. It's not
a strict plan as the development of Orn is still in early stages, but it gives
a well scoped idea of how it is the view for Orn.

## Match and if expressions

If and match statements become valid expressions:

```orn
let x = if cond {...} else {...}
```

```orn
let msg = match (result) {
	Ok(n) => { format(n) }
	Err(e) => { log(e); "error" }
	_ => { "unknown" }
};
```

Not like ternaries, these expressions allow for logic inside them.

## null safety

Optional types to prevent null pointer dereferences. Not like Rust safety, here
pointers can be null, but the code guides the programmer to handle nullability.

```orn
let p: ?*int = get_ptr();
// gotta check if p is null before deref
```

## Methods and self keyword

The `self` keyword is a great way to make the code more readable this allows
enums and structs to have methods different from static functions:

```orn
struct Point {
	x: int;
	y: int;
}

impl Point {
	fn translate(*self, dx: int, dy: int) {
		self.x += dx;
		self.y += dy;
	}
}

let p = Point { x: 10, y: 20 };
p.translate(5, 10);
```


## Generics and traits

```orn
trait Printable {
	fn print(*self) -> void;
}

fn show<T: Printable>(x: T) {
	x.print();
}

struct Point {
	x: int;
	y: int;
}

impl Point : Printable {
	fn print(*self) -> void {
		// ...
	}
}

let p = Point { x: 10, y: 20 };
show(p);

```

traits are a way to set contracts for Generics, instead of being just "anything"
they are "anything that implements this trait".

## Decorators and preprocessor

```orn

@inline
fn foo //...
```
```orn
@pre(x >= 0)
@post(ret >= 0)
fn foo(x: int) -> int {
	x * 2
}
```

This decorators are just ideas of what it could be; for `@pre` would make x
range different for the scope as the range after `@pre(x > 0)` would change from
[-INT_MAX, INT_MAX] to [0, INT_MAX] which fits inside `unsigned` without data
loss, and making the compiler know that.

other decorators ideas are:

- `@deprecated` - throw a warning when used.
- `@debug` - execute an additional code when --debug flag is set.
- `@use` - instead of the trait being in the `impl`, this goes above a impl,
  following the previous example:

```orn
@use(Printable)
impl Point {
	fn print(*self) -> void {
		// ...
	}
}
```

## Closures and lambdas

Anonymous functions that can be assigned to variables, be a type or passed as
arguments:

```orn
let add = (a: int, b: int) -> int { a + b };
```
