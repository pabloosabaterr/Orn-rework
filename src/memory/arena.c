#include "memory/arena.h"
#include "memory/wrapper.h"
#include <stdlib.h>
#include <string.h>

static struct arena_chunk *arena_chunk_new(size_t size)
{
	struct arena_chunk *chunk = xmalloc(sizeof(*chunk));
	chunk->size = size;
	chunk->ptr = xmalloc(size);
	chunk->next = NULL;
	return chunk;
}

void arena_init(struct arena *arena, size_t init)
{
	struct arena_chunk *chunk = arena_chunk_new(init);
	arena->buffer = chunk;
	arena->head = chunk;
	arena->offset = 0;
	arena->next_buffer_size = init;
}

void *arena_alloc(struct arena *arena, size_t size)
{
	void *ptr;
	size_t aligned_offset = align_up(arena->offset, sizeof(void *));
	size_t chunk_size = arena->next_buffer_size;

	if (aligned_offset + size <= arena->head->size) {
		ptr = arena->head->ptr + aligned_offset;
		arena->offset = aligned_offset + size;
		return ptr;
	}

	if (chunk_size < size)
		chunk_size = size;

	struct arena_chunk *chunk = arena_chunk_new(chunk_size);
	arena->head->next = chunk;
	arena->head = chunk;
	arena->offset = size;
	arena->next_buffer_size = alloc_needed(chunk_size);

	return chunk->ptr;
}

void *arena_realloc(struct arena *arena, void *ptr, size_t old_size, size_t new_size)
{
	void *new_ptr;

	if (ptr && (char *)ptr + old_size == arena->head->ptr + arena->offset) {
		size_t start = (char *)ptr - arena->head->ptr;
		if (start + new_size <= arena->head->size) {
			arena->offset = start + new_size;
			return ptr;
		}
	}

	new_ptr = arena_alloc(arena, new_size);
	if (ptr && old_size)
		memcpy(new_ptr, ptr, old_size < new_size ? old_size : new_size);
	return new_ptr;
}

void arena_reset(struct arena *arena)
{
	arena->head = arena->buffer;
	arena->offset = 0;
}

void arena_free(struct arena *arena)
{
	struct arena_chunk *chunk = arena->buffer;
	while (chunk) {
		struct arena_chunk *next = chunk->next;
		free(chunk->ptr);
		free(chunk);
		chunk = next;
	}

	arena->buffer = NULL;
	arena->head = NULL;
	arena->offset = 0;
	arena->next_buffer_size = 0;
}
