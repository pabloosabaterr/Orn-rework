CC = gcc
CFLAGS = -std=c11 -Wall -Werror -Wextra -pedantic -I src -I build

NAME = orn
SRC = $(wildcard src/**/*.c) $(wildcard src/*.c)

all:
	mkdir -p build/ && $(CC) $(CFLAGS) $(SRC) -o build/$(NAME)

run: all
	./build/$(NAME) $(ARGS)

PROVE = prove
T = $(wildcard tests/t[0-9]*.sh)

test: all
	$(PROVE) --exec sh -j$$(nproc) $(T)

vtest: all
	$(PROVE) --exec sh --directives $(T)

shtest: all
	@for t in $(T); do echo "$$t"; sh "$$t" || exit 1; done

format:
	clang-format -i $(SRC)

check-format:
	clang-format --dry-run --Werror $(SRC) && \
	! grep -Pn '.{81}' --include='*.md' -r . | grep -v 'http\|badge'

work:
	grep -r "NEEDSWORK" src/ docs/src/NEEDSWORK.md --color=always

clean:
	rm -rf build/
