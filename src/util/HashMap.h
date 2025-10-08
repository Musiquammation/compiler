#ifndef COMPILER_UTIL_HASHMAP_H_
#define COMPILER_UTIL_HASHMAP_H_


typedef struct {
	int key;
	int used;
	void *value;
} HashMapEntry;

typedef struct {
	HashMapEntry *entries;
	int capacity;
	int size;
} HashMap;

// Création d'une nouvelle hash map
void HashMap_create(HashMap* map, int initialCapacity);

// Libère la mémoire
void HashMap_free(HashMap *map, void(*delete)(void*ptr));

// Récupère la valeur associée à une clé, ou NULL si absente
void* HashMap_get(HashMap *map, int key);

// Récupère la valeur, ou l'insère si absente (via callback create)
void* HashMap_get_or_insert(HashMap *map, int key, void*(*create)(int key, void* ctx), void* ctx);


#endif