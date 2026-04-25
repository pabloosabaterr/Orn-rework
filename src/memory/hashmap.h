#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>

/*
 * Generic hash table with containers.
 *
 * Instead of storing opaque pointers, each entry embeds a struct hashmap_entry
 * as the first member. example:
 *
 * struct example_entry {
 *         struct hashmap_entry entry;
 *         struct data data;
 * };
 *
 * static int foo_cmp(...)
 * {
 *         ...
 * }
 *
 * To insert an entry:
 *
 * struct example_entry *e = arena_alloc(&arena, sizeof(*e));
 * e->data = ...;
 * e->entry.hash = strhash(ptr, len);
 * hashmap_put(&map, &e->entry);
 *
 *
 * Lookup an entry:
 *
 * unsigned int h = strhash(ptr, len);
 * struct hashmap_entry *entry = hashmap_get(&map, h, foo_cmp, key);
 * if (entry) {
 *         struct example_entry *e = (struct example_entry *)entry;
 *         ...
 * }
 *
 * The entries are owned by the caller and stored at the arena, while the
 * bucket array is owned by the hashmap and needs to be freed after.
 */

struct hashmap_entry {
	struct hashmap_entry *next;
	unsigned int hash;
};

struct hashmap {
	struct hashmap_entry **table;
	size_t size;
	size_t nr;
};

typedef int (*hashmap_cmp_fn)(const struct hashmap_entry *entry, const void *key);

void hashmap_init(struct hashmap *map);
void hashmap_free(struct hashmap *map);
void hashmap_put(struct hashmap *map, struct hashmap_entry *entry);
unsigned int strhash(const char *start, size_t len);
struct hashmap_entry *hashmap_get(struct hashmap *map, unsigned int hash,
				  hashmap_cmp_fn cmp, const void *key);

#endif
