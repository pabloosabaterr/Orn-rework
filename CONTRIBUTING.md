# Contributing

Thanks for your interest in contributing!

There are many ways to contribute to Orn, from reporting bugs and suggesting
what you'd like to have implemented, to writing code and documentation.
All contributions are welcome.

To make it easier for new folks to contribute, throughout the code there are
`NEEDSWORK` tags, that can be found with `make work` and they are a
good place to start looking where to help.
Most of them are improvements that 'work' but could be better, such as an array
that is linearly searched that could be replaced with hash table, or functions
that start to be too big to be maintained properly.

Another way to find where to contribute is running `make vtest`, known failing
tests will show up. e.g.:

```
tests/t0001-lexer.sh ........ 1/?
not ok 14 - invalid escape sequence should be rejected # TODO known failure
```
This failing hints usually have a `NEEDSWORK` tag that helps to find where you
should look at to fix it along with a more detailed explanation.

## Starting with the code

1. Fork the repo and clone your fork.

2. Create a topic branch from `main` : `git checkout -b name-initials/topic`
   e.g.: my name is Pablo Sabater, so I would run:
   `git checkout -b ps/fix-some-bug`

3. Make your changes, run `make check-format` and make sure that test pass.
4. Commit followin the guidelines below.
5. Push to your fork and open a PR.

Branch topic names should be short and descriptive: `fix-something`,
`add-cool-feature`, `improve-x`.

If the PR is related to an issue, please add a reference to the issue at the
end of the PR description, e.g.: `Closes #issue-number`.

It is a good practice if the topic is too big to split it into smaller PRs.

## How do I run stuff?

There are two ways to run the compiler:

- `make`: build the compiler at `./build/orn` and the you can run it with
  `./build/orn <test-file.orn> <flags>`.

- `make run ARGS="<test-file.orn> <flags>"`: this is a shortcut for the above,
  it will build the compiler and run it with the given flags.

## How do I test?

There are a few testing commands at the Makefile:

- `test`: Runs all the tests in parallel, shows all the tests. but skips all the
  failure hints.

- `vtest`: Runs all the tests NOT in parallel, shows all the tests and also
  shows the fail hints, so you can find known failures.

- `shtest`: This is the fallback for when the above commands fail. Runs all the
  tests with `sh` and shows all the tests.

## I sent a PR and got changes requested, now what?

Please after receiving feedback, try to amend the commit instead of a follow-up
fix commit. The correct approach while a series is being reviewed is to
`git commit --amend` the commits where the fixes are needed, you can also use
`git rebase -i` to amend commits that are not the last one.

It's not the same if you find something that needs to be fixed after it has
been merged, in that case you should just send a new PR with the fix.

## Guidelines

- Read [CodingGuidelines.md](CodingGuidelines.md) before writing code.

- Run `make check-format` and `make test` before submitting.

- Commit messages: imperative mood, 72 chars max for the subject line.
  Try to explain the "why" it is needed or the problem suffered, then explain
  the "what" it does to solve it and finally the "how".
  About the "how" it's not needed technical details, those can be seen already
  in the code; if a technical detail needs to be explained it should be
  commented in the code itself (removing the need to explain in the commit
  message). Explain the "how" in present tense and avoid 1st person in a
  subjective explanation.

> **AI-generated content:**
>
> It is valued contributions from people who engage the project with their own
> ideas and knowledge, while it is not forbidden to use AI as a tool to assist
> you, help on debugging, generate test cases, etc. The content should always be
> reviewed and not just a copy-paste. The commit message should be completely
> be written by the contributor, excessive boilerplate or generic ai-generated
> messages will be rejected.

## Contribution

Unless you explicitly state otherwise, any contribution intentionally
submitted for inclusion in Orn by you, as defined in the Apache-2.0
license, shall be dual licensed as above, without any additional terms
or conditions.
