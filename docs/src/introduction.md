# Introduction

This book is the main reference for the Orn programming language.

> **Note**:
> For bugs or missings in this book please report an issue in the
> [Github repo](https://github.com/pabloosabaterr/Orn-rework)

## What is Orn?

Orn is a language that aims to look similar to typescript with more
low-level capabilities along with clear errors.

Orn aims to be a low-level language that attracts coders from high-level langs
like typescript but keeping the things that low-level coders like, providing a
soft learning curve and a good developer experience.

## Release

Orn is still being designed and implemented.

## Conventions throughout the book

thoughout the book, you'll find many syntax explanations and code snippets.
The syntax explanations are BNF-like rules following this notation:

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
example:

```
	if_stmt = IF expr stmt (ELIF expr stmt)* (ELSE stmt)?
```

This means that an `if_stmt` consists of an `TK_IF` followed by an `expr` and a
`stmt`, followed by zero or more groups of `TK_ELIF expr stmt`, and optionally
an else statement that would be the group `TK_ELSE stmt`.

---

**Note** Orn is currently being designed and developed, so it can be that some
of the syntax in this book it is not implemented yet, but it is intended to be.
