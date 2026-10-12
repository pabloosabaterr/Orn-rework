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
WARN += -Wimplicit-fallthrough

CFLAGS = -std=c11 $(WARN) -MMD -MP -I src
LDFLAGS =

CFLAGS += $(DFLAGS)

SAN_FLAGS = -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1

NAME = orn
TARGET = build/$(NAME)

SRC  = src/main.c

SRC += src/driver/orn.c
SRC += src/driver/compiler.c

SRC += src/frontend/lexer.c
SRC += src/frontend/parser.c
SRC += src/frontend/diagnostic.c

SRC += src/support/arena.c
SRC += src/support/hashmap.c
SRC += src/support/str-buf.c
SRC += src/support/wrapper.c
SRC += src/support/log.c
SRC += src/support/io.c
SRC += src/support/parse-options.c

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

test:
	cd tests && \
    sh runner.sh

work:
	grep -rn "NEEDSWORK" src/ --color=always

# Cada capa solo puede incluir capas inferiores: support < frontend < driver
layers:
	@! grep -rnE '#include "(frontend|driver)/' src/support/ || \
		{ echo "support/ no puede depender de frontend/ ni driver/"; exit 1; }
	@! grep -rnE '#include "driver/' src/frontend/ || \
		{ echo "frontend/ no puede depender de driver/"; exit 1; }

clean:
	rm -rf build/

-include $(DEP)

.DELETE_ON_ERROR:
FORCE:
.PHONY: all run test san work layers clean FORCE
