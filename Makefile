CC = gcc
CFLAGS = -Wall -Werror -Wextra -pedantic -I src -I build

NAME = orn
SRC = src/main.c

all:
	mkdir -p build/ && $(CC) $(CFLAGS) $(SRC) -o build/$(NAME)

run: all
	./build/$(NAME) $(ARGS)

PROVE = prove
T = $(wildcard tests/*.sh)

test: all
	$(PROVE) --exec sh -j$$(nproc) $(T)

test-sh: all
	@for t in $(T); do echo "$$t"; sh "$$t" || exit 1; done

format:
	clang-format -i $(SRC)

check-format:
	clang-format --dry-run --Werror $(SRC) && \
	! grep -Pn '.{81}' --include='*.md' -r . | grep -v 'http\|badge'

clean:
	rm -rf build/