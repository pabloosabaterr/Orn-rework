CC = gcc

WARN  = -Wall -Wextra -Werror -pedantic
WARN += -Wformat=2
WARN += -Wshadow
WARN += -Wstrict-prototypes
WARN += -Wmissing-prototypes
WARN += -Wredundant-decls
WARN += -Wundef
WARN += -Wpointer-arith
WARN += -Wcast-align
WARN += -Wvla
WARN += -Wsign-compare

CFLAGS = -std=c11 $(WARN) -MMD -MP -I src
LDFLAGS =

CFLAGS += $(DFLAGS)

SAN_FLAGS = -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1

NAME = orn
TARGET = build/$(NAME)

SRC  = src/main.c
SRC += src/compiler.c
SRC += src/diagnostic.c
SRC += src/lexer.c
SRC += src/arena.c
SRC += src/hashmap.c
SRC += src/str-buf.c
SRC += src/wrapper.c
SRC += src/log.c
SRC += src/parser.c
SRC += src/parse-options.c

OBJ = $(SRC:src/%.c=build/obj/%.o)
DEP = $(OBJ:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJ) | build
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

build/obj/%.o: src/%.c build/.flags
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

build/.flags: FORCE | build
	@echo '$(CFLAGS)' | cmp -s - $@ || echo '$(CFLAGS)' > $@

build:
	mkdir -p build

run: all
	./$(TARGET) $(ARGS)

san: clean
	$(MAKE) DFLAGS="$(SAN_FLAGS)"

san-test: san
	UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
	$(PROVE) --exec sh $(T)

PROVE = prove
T = $(wildcard tests/t[0-9]*.sh)

test: all
	$(PROVE) --exec sh $(T)

vtest: all
	$(PROVE) --exec sh --directives $(T)

shtest: all
	@for t in $(T); do echo "$$t"; sh "$$t" || exit 1; done

work:
	grep -r "NEEDSWORK" src/ docs/src/NEEDSWORK.md --color=always

clean:
	rm -rf build/

-include $(DEP)

.DELETE_ON_ERROR:
FORCE:
.PHONY: all run test vtest shtest san san-test format check-format \
	work clean FORCE
