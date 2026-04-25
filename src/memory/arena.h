#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

#define align_up(n, alignement) (((n) + (alignement) - 1) & ~((alignement) -1))

#define ARENA_ALLOC_GROW(arena, ptr, count, alloc) \
	do { \
		if ((count) > (alloc)) { \
			size_t new_alloc = (alloc); \
			if(alloc_needed(alloc) < (count)) \
				new_alloc = (count); \
			else \
				new_alloc = alloc_needed(new_alloc); \
			(ptr) = arena_realloc((arena), (ptr), \
					      sizeof(*(ptr)) * (alloc), \
					      sizeof(*(ptr)) * (new_alloc)); \
			(alloc) = new_alloc; \
		} \
	} while (0)

struct arena_chunk {
	size_t size;
	struct arena_chunk *next;
	char *ptr;
};

struct arena {
	size_t offset;
	struct arena_chunk *buffer;
	struct arena_chunk *head;
	size_t next_buffer_size;
};

/* arena functions */

void arena_init(struct arena *arena, size_t init);
/*
 * Allocates 'size' bytes from the arena and returns a pointer to the allocated
 * memory.
 * size must be > 0.
 */
void *arena_alloc(struct arena *arena, size_t size);
void *arena_realloc(struct arena *arena, void *ptr, size_t old_size, size_t new_size);
void arena_reset(struct arena *arena);
void arena_free(struct arena *arena);


#endif
