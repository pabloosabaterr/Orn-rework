# Introduction

This book is the main documentation for the
[Orn](https://github.com/pabloosabaterr/Orn-rework) programming language.

> [!NOTE]
> For bugs or missing content in this book please open an issue in the
> [GitHub repo](https://github.com/pabloosabaterr/Orn-rework/issues).

> [!WARNING]
> Orn is being designed and developed. Some of the syntax in this book may not
> be implemented yet.

## What is Orn?

Orn is a systems language where every type is a range. Instead of choosing
between `int`, `u8` or `bool`, a value is described by the integers it can
hold, written `lo..hi` with both ends inclusive.

```
age : 0..150 = 30;
```

The compiler keeps two ranges for every value:

- The **declared range** fixes how the value is stored. It never changes.
- The **known range** is what the compiler can prove at each point of the
  program. Comparisons make it smaller.

```
classify :: (i: 0..1000) 0..2 {
    if i < 10 {
        // known range of i: 0..9
        0
    } else if i < 100 {
        // known range of i: 10..99
        1
    } else {
        // known range of i: 100..1000
        2
    }
};
```

## Types are aliases

The usual type names are ordinary constant bindings in the standard library:

```
u8   :: 0..255;
i32  :: -2147483648..2147483647;
bool :: 0..1;
```

`u8` and `0..255` are the same thing. A value can be passed wherever the
expected range contains its own.

## No silent overflow

Arithmetic follows the ranges of its operands. Adding two `u8` gives `0..510`,
and storing that in a `u8` is a compile error. Wrapping or saturating has to be
asked for explicitly.

When a value comes from outside and its range is unknown, `in` checks it at
runtime and narrows it:

```
age :: (n: -32768..32767) 0..150 {
    n in 0..150 else { ret 0; }
};
```

## Limits

Ranges are checked with interval arithmetic, which treats operands as
independent. Some correct programs are rejected: `x * x` with `x : -3..3` is
`-9..9`, not `0..9`. The `examples/errors` folder in the repository collects
these cases.
