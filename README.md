# Orn rework

<p align="center">
	<img src="assets/ORN.png" alt="Orn Lang Logo" width="120">
</p>

Orn is a language that aims to be look similar to typecript but with a more
low-level capabilities along with clear errors.

## Introduction

Orn aims to be a low-level language that attracts coders from high-level langs
like typescript but keeping the things that low-level coders like, providing a
soft learning curve and a good developer experience.

This is a rework of the original Orn compiler project, where it was full
with bad design decisions and a lot of technical debt, but once learned a lot,
this rework aims for a much better design, cleaner and more maintainable
codebase.

## Architecture

- [Lexer](DOCUMENTATION/lexer.md): tokenization and lexical analysis
- [Grammar spec](DOCUMENTATION/parser.md):
 the syntax and semantics of the language

## Build

```
make
```

## Usage

```
make run ARGS="..."
```

## Tests

```
make test           # parallel via prove
make test-sh        # fallback without prove
```

Uses `prove` with parallel execution if available, falls back to
running each script with `sh`. Tests live in `tests/`.

## Code Style

This project follows the
[Git coding style](https://github.com/git/git/blob/master/Documentation/CodingGuidelines).
Formatting is enforced via `clang-format`.
See [CodingGuidelines.md](CodingGuidelines.md) for details.

```
make format         # format all source files
make check-format   # check without modifying (for CI)
```

## Clean

```
make clean
```

## Contributing

See [Contributing.md](DOCUMENTATION/Contributing.md).

## License

[MIT](LICENSE)
