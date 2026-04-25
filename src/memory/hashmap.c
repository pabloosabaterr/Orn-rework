#include "memory/hashmap.h"
#include "memory/wrapper.h"
#include <stdlib.h>
#include <string.h>

void hashmap_init(struct hashmap *map)
{
	memset(map, 0, sizeof(*map));
}

static void hashmap_grow(struct hashmap *map)
{
	size_t old_size = map->size;
	struct hashmap_entry **old_table = map->table;
	size_t new_size = old_size * 2;
	struct hashmap_entry **new_table;
	size_t i;

	new_table = xcalloc(new_size, sizeof(*new_table));

	for (i = 0; i < old_size; i++) {
		struct hashmap_entry *entry = old_table[i];
		while (entry) {
			struct hashmap_entry *next = entry->next;
			unsigned int bucket = entry->hash % new_size;

			entry->next = new_table[bucket];
			new_table[bucket] = entry;
			entry = next;
		}
	}

	free(old_table);
	map->table = new_table;
	map->size = new_size;
}

void hashmap_put(struct hashmap *map, struct hashmap_entry *entry)
{
	unsigned int bucket;

	if (!map->table) {
		map->size = 16;
		map->table = xcalloc(map->size, sizeof(*map->table));
	} else if (map->nr >= map->size) {
		hashmap_grow(map);
	}

	bucket = entry->hash % map->size;
	entry->next = map->table[bucket];
	map->table[bucket] = entry;
	map->nr++;
}

struct hashmap_entry *hashmap_get(struct hashmap *map, unsigned int hash,
				  hashmap_cmp_fn cmp, const void *key)
{
	struct hashmap_entry *entry;
	unsigned int bucket;

	if (!map->table)
		return NULL;

	bucket = hash % map->size;
	entry = map->table[bucket];

	while (entry) {
		if (entry->hash == hash && !cmp(entry, key))
			return entry;
		entry = entry->next;
	}

	return NULL;
}

/*
 * 32 bit FNV-1a hash function
 *
 * see documentation about it:
 * http://www.isthe.com/chongo/tech/comp/fnv/#FNV-1a
 */
unsigned int strhash(const char *str, size_t len)
{
	unsigned int hash = 2166136261u;
	size_t i;

	for (i = 0; i < len; i++) {
		hash ^= (unsigned char)str[i];
		hash *= 16777619u;
	}

	return hash;
}

void hashmap_free(struct hashmap *map)
{
	free(map->table);
	map->table = NULL;
	map->size = 0;
	map->nr = 0;
}
