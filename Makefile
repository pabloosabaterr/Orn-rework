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

ifdef DEBUG
	CFLAGS += -g -O0
endif
ifdef TRACES
	CFLAGS += -DTRACES -g
endif

SAN_FLAGS = -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1

NAME = orn
TARGET = build/$(NAME)

SRC  = src/main.c
SRC += src/compiler.c
SRC += src/diagnostic/diagnostic.c
SRC += src/lexer/lexer.c
SRC += src/memory/arena.c
SRC += src/memory/hashmap.c
SRC += src/memory/str-buf.c
SRC += src/memory/wrapper.c
SRC += src/utils/log.c

HDR = $(wildcard src/*.h src/*/*.h)

ALL_C   = $(wildcard src/*.c src/*/*.c)
MISSING = $(filter-out $(ALL_C),$(SRC))

OBJ = $(SRC:src/%.c=build/obj/%.o)
DEP = $(OBJ:.o=.d)

all: check-src $(TARGET)

$(TARGET): $(OBJ) | build
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

build/obj/%.o: src/%.c build/.flags
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

build/.flags: FORCE | build
	@echo '$(CFLAGS)' | cmp -s - $@ || echo '$(CFLAGS)' > $@

build:
	mkdir -p build

check-src:
	@test -z "$(MISSING)" || { \
		echo "make: listed in SRC but not found: $(MISSING)"; exit 1; }

run: all
	./$(TARGET) $(ARGS)

debug: DFLAGS += -g -O0
debug: clean all

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

format:
	clang-format -i $(ALL_C) $(HDR)

check-format:
	clang-format --dry-run --Werror $(ALL_C) $(HDR) && \
	! grep -Pn '.{81}' --include='*.md' -r . | grep -v 'http\|badge'

work:
	grep -r "NEEDSWORK" src/ docs/src/NEEDSWORK.md --color=always

clean:
	rm -rf build/

-include $(DEP)

.DELETE_ON_ERROR:
FORCE:
.PHONY: all check-src run test vtest shtest san san-test format check-format \
	work clean FORCE
