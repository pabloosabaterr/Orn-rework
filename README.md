# Orn

<p align="center">
	<img src="assets/ORN.png" alt="Orn Lang Logo" width="120">
</p>

Orn is a language inspired by TypeScript-like syntax but with low-level
capabilities along with clear errors and manual memory management.

## Introduction

Orn is a systems-oriented language that aims for developers coming from
high-level languages such as TypeScript but also low-level devs that are tired
of the boilerplate and unreadable code.

Orn brings familiar syntax with low-level capabilities like manual memory
management, pointers and strong static types while keeping a soft learning curve
and a good developer experience.

## Examples

**Manual memory**
```
struct Stack {
	data: *int;
	len: int;
}

impl Stack {
	fn push(s: *Stack, val: int) {
		s.data[s.len] = val;
		s.len++;
	}

	fn pop(s: *Stack) -> int {
		s.len--;
		return s.data[s.len];
	}
}
```
**Tagged unions**
```
enum Shape {
	Circle(int),
	Rect(int, int),
}

fn area(s: Shape) -> int {
	match s {
		Circle(r) => { return r * r * 3; }
		Rect(w, h) => { return w * h; }
	}
}
```

Take a look at the [Orn Book](https://pabloosabaterr.github.io/Orn-rework/)
for documentation about Orn.

## Status

Orn is currently being developed, and it is in a very early stage.

## Architecture

![Compiler pipeline](assets/compiler-pipeline.svg)

- [Lexer](DOCUMENTATION/lexer.md): tokenization and lexical analysis
- [Grammar spec](DOCUMENTATION/parser.md):
 the syntax and semantics of the language

for information about the semantic phase see
[Semantic Pass](https://pabloosabaterr.github.io/Orn-rework/semantic-pass.html)

## Build

`make` builds the compiler at `./build/orn`.

You can run it with: `./build/orn <test-file.orn> <flags>`

You can skip the two steps above with: `make run ARGS="<test-file.orn> <flags>"`

## Tests

```
make test           # parallel via prove
make vtest          # non-parallel with fail hints
make shtest         # fallback without prove
```
More info about testing in [Contributing](CONTRIBUTING.md).

## Code Style

This project follows the
[Git coding style](https://github.com/git/git/blob/master/Documentation/CodingGuidelines).
Formatting is enforced via `clang-format`.
See [CodingGuidelines.md](CodingGuidelines.md) for details.

```
make format         # format all source files
make check-format   # check without modifying (for CI)
```

## Contributing

All kinds of contributions are welcome: bug reports, features, documentation,
tests, etc.

See [Contributing](CONTRIBUTING.md).

### Contribution License

Unless you explicitly state otherwise, any contribution intentionally
submitted for inclusion in Orn by you, as defined in the Apache-2.0
license, shall be dual licensed as above, without any additional terms
or conditions.

## License

Licensed under either of

- Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE) or
  <http://www.apache.org/licenses/LICENSE-2.0>)
- MIT license ([LICENSE-MIT](LICENSE-MIT) or
  <http://opensource.org/licenses/MIT>)

at your option.
