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

/*
 * NEEDSWORK: resolve checkpoint-endings fragmentation by having an array of
 * left over size and the pointer to the last offset of the chunk.
 * This way if something fits in the leftover memory it gets stored in a previous
 * chunk instead of the current one.
 * Having n chunks this new array should have only "left" chunks thus it
 * must have n-1 elements; If an allocation fits inside a left chunk, the
 * entry should be edited instead of adding a new one. THE NEW SIZE MUST BE
 * ALIGNED
 */
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
