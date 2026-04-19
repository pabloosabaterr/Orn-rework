# Introduction

This book is the main documentation for the
[Orn](https://github.com/pabloosabaterr/Orn-rework) programming language.

> [!NOTE]
> For bugs or missings in this book please report an issue in the
> [Github repo](https://github.com/pabloosabaterr/Orn-rework/issues)

> [!WARNING]
> Orn is currently being designed and developed, so it can be that some
> of the syntax in this book it is not implemented yet, but it is intended to
> be.

## What is Orn?

Orn is a systems-oriented language that aims for developers coming from
high-level languages such as TypeScript but also low-level devs that are tired
of the boilerplate and unreadable code.

Orn brings familiar syntax with low-level capabilities like manual memory
management, pointers and strong static types while keeping a soft learning curve
and a good developer experience.

### Release

Orn is still being designed and implemented. This documentation works as a
declaration of what it is intended to be for v1.0.

For a roadmap after v1.0, see the [Roadmap](NEEDSWORK.md) section.

## How does Orn look like?

```orn
struct Node {
	val: int;
	next: *Node;
}

enum Option {
	Some(int),
	None,
}

impl Node {
	fn create(val: int) -> Node {
		ret Node { val: val, next: null };
	}

	fn find(head: *Node, target: int) -> Option {
		while head {
			if head.val == target {
				ret ::Some(head.val);
			}
			head = head.next;
		}
		ret ::None;
	}
}

fn main() -> int {
	let a = Node::create(10);
	let b = Node::create(20);
	let c = Node::create(30);
	a.next = &b;
	b.next = &c;

	let result = Node::find(&a, 20);

	match (result) {
		Some(val) => { ret val; }
		None => { ret -1; }
	}
}
```

This example is consist of linked list implementation along with a function
to find a value in the list.

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
e.g:

```
	if_stmt = IF expr stmt (ELIF expr stmt)* (ELSE stmt)?
```

This means that an `if_stmt` consists of an `TK_IF` followed by an `expr` and a
`stmt`, followed by zero or more groups of `TK_ELIF expr stmt`, and optionally
an else statement that would be the group `TK_ELSE stmt`.

## Contributing

There are many ways to contribute to Orn, from reporting bugs and suggesting
what you'd like to have implemented, to writing code and documentation.
All contributions are welcome.

You can know more about how to contribute at
[Contributing to Orn](https://github.com/pabloosabaterr/Orn-rework/blob/main/CONTRIBUTING.md)
