#include "HashMap.h"

#include <stdlib.h>
#include <string.h>



// --- Hash utilitaire ---
static int hash_int32(int x) {
	x ^= x >> 16;
	x *= 0x7feb352d;
	x ^= x >> 15;
	x *= 0x846ca68b;
	x ^= x >> 16;
	return x;
}

// --- Création ---
void HashMap_create(HashMap* map, int initialCapacity) {
	int cap = 1;
	while (cap < initialCapacity) cap <<= 1;

	map->capacity = cap;
	map->size = 0;
	map->entries = calloc(cap, sizeof(HashMapEntry));
}

// --- Libération ---
void HashMap_free(HashMap *map, void(*delete)(void* ptr)) {
	if (!map) return;
	
	if (delete) {
		for (int i = 0; i < map->capacity; i++) {
			if (map->entries[i].used && map->entries[i].value) {
				delete(map->entries[i].value);
			}
		}
	}

	free(map->entries);
}

// --- Redimensionnement ---
static void HashMap_resize(HashMap *map, int new_capacity) {
	HashMapEntry* old_entries = map->entries;
	int old_cap = map->capacity;

	map->capacity = new_capacity;
	map->entries = calloc(new_capacity, sizeof(HashMapEntry));
	map->size = 0;

	for (int i = 0; i < old_cap; i++) {
		if (old_entries[i].used) {
			int h = hash_int32(old_entries[i].key);
			int idx = h & (map->capacity - 1);
			while (map->entries[idx].used) {
				idx = (idx + 1) & (map->capacity - 1);
			}
			map->entries[idx] = old_entries[i];
			map->size++;
		}
	}
	free(old_entries);
}

// --- Lookup simple ---
void* HashMap_get(HashMap *map, int key) {
	if (!map) return NULL;

	int h = hash_int32(key);
	int idx = h & (map->capacity - 1);

	while (map->entries[idx].used) {
		if (map->entries[idx].key == key) {
			return map->entries[idx].value;
		}
		idx = (idx + 1) & (map->capacity - 1);
	}
	return NULL;
}

// --- Lookup ou insertion ---
void* HashMap_get_or_insert(HashMap *map, int key, void *(*create)(int value, void* ctx), void* ctx) {
	int h = hash_int32(key);
	int idx = h & (map->capacity - 1);

	while (map->entries[idx].used) {
		if (map->entries[idx].key == key) {
			return map->entries[idx].value;
		}
		idx = (idx + 1) & (map->capacity - 1);
	}

	// Insérer
	void *val = create(key, ctx);
	map->entries[idx].used = 1;
	map->entries[idx].key = key;
	map->entries[idx].value = val;
	map->size++;

	if (map->size * 2 > map->capacity) { // load factor > 0.5
		HashMap_resize(map, map->capacity * 2);
	}

	return val;
}
