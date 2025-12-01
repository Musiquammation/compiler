#include "primitives.h"
#include "globalLabelPool.h"
#include "Prototype.h"

#include <string.h>
#include <stdlib.h>

PrimitiveContainer _primitives;

// comparaison par pointeur (pas strcmp)
static int cmpLabelPtr(const void* a, const void* b) {
	const label_t* la = (const label_t*)a;
	const label_t* lb = (const label_t*)b;
	if (*la < *lb) return -1;
	if (*la > *lb) return 1;
	return 0;
}

void primitives_init(void) {
	#define name(id, s) _commonLabels._##id##s
	#define def(id, s, k, namemethod) \
		_primitives.class_##id##s.name = namemethod(id, s); \
		_primitives.class_##id##s.meta = NULL; \
		_primitives.class_##id##s.definitionState = DEFINITIONSTATE_DONE; \
		_primitives.class_##id##s.primitiveSizeCode = k; \
		_primitives.class_##id##s.isRegistrable = true; \
		_primitives.class_##id##s.size = s/8; \
		_primitives.class_##id##s.maxMinimalSize = s/8; \
		_primitives.proto_##id##s.mode = PROTO_MODE_PRIMITIVE; \
		_primitives.proto_##id##s.primitive.cl = &_primitives.class_##id##s; \
		_primitives.proto_##id##s.primitive.size = s/8; \
		_primitives.proto_##id##s.primitive.sizeCode = k; \
		_primitives.type_##id##s.proto = &_primitives.proto_##id##s; \
		_primitives.type_##id##s.primitiveSizeCode = k;\
		_primitives.type_##id##s.data = NULL;\
		
	def(i, 8,  -1,  name);
	def(u, 8,  1,   name);
	def(i, 16, -2,  name);
	def(u, 16, 2,   name);
	def(i, 32, -4,  name);
	def(u, 32, 4,   name);
	def(i, 64, -8,  name);
	def(u, 64, 8,   name);
	def(f, 32, 126, name);
	def(f, 64, 127, name);

	#undef def
	#undef name

	struct {
		label_t lbl;
		Class* cls;
		Prototype* proto;
		Type* type;
	} tmp[] = {
		{ _commonLabels._i8,     &_primitives.class_i8,    &_primitives.proto_i8,    &_primitives.type_i8 },
		{ _commonLabels._u8,     &_primitives.class_u8,    &_primitives.proto_u8,    &_primitives.type_u8 },
		{ _commonLabels._i16,    &_primitives.class_i16,   &_primitives.proto_i16,   &_primitives.type_i16 },
		{ _commonLabels._u16,    &_primitives.class_u16,   &_primitives.proto_u16,   &_primitives.type_u16 },
		{ _commonLabels._i32,    &_primitives.class_i32,   &_primitives.proto_i32,   &_primitives.type_i32 },
		{ _commonLabels._u32,    &_primitives.class_u32,   &_primitives.proto_u32,   &_primitives.type_u32 },
		{ _commonLabels._i64,    &_primitives.class_i64,   &_primitives.proto_i64,   &_primitives.type_i64 },
		{ _commonLabels._u64,    &_primitives.class_u64,   &_primitives.proto_u64,   &_primitives.type_u64 },
		{ _commonLabels._f32,    &_primitives.class_f32,   &_primitives.proto_f32,   &_primitives.type_f32 },
		{ _commonLabels._f64,    &_primitives.class_f64,   &_primitives.proto_f64,   &_primitives.type_f64 },

		// alias
		{ _commonLabels._char,   &_primitives.class_i8,    &_primitives.proto_i8,    &_primitives.type_i8  },
		{ _commonLabels._bool,   &_primitives.class_u8,    &_primitives.proto_u8,    &_primitives.type_u8  },
		{ _commonLabels._short,  &_primitives.class_i16,   &_primitives.proto_i16,   &_primitives.type_i16 },
		{ _commonLabels._int,    &_primitives.class_i32,   &_primitives.proto_i32,   &_primitives.type_i32 },
		{ _commonLabels._long,   &_primitives.class_i64,   &_primitives.proto_i64,   &_primitives.type_i64 },
		{ _commonLabels._float,  &_primitives.class_f32,   &_primitives.proto_f32,   &_primitives.type_f32 },
		{ _commonLabels._double, &_primitives.class_f64,   &_primitives.proto_f64,   &_primitives.type_f64 },
		{ _commonLabels._size_t, &_primitives.class_u64,   &_primitives.proto_u64,   &_primitives.type_u64 },
	};

	enum {N = sizeof(tmp)/sizeof(tmp[0])};

	// extraire juste les labels pour trier
	label_t labels[N];
	for (int i = 0; i < N; i++)
		labels[i] = tmp[i].lbl;

	qsort(labels, N, sizeof(label_t), cmpLabelPtr);

	// reconstruire sortedLabels, sortedClasses, sortedTypes
	for (int i = 0; i < N; i++) {
		_primitives.sortedLabels[i] = labels[i];
		for (int j = 0; j < N; j++) {
			if (tmp[j].lbl == labels[i]) {
				_primitives.sortedClasses[i] = tmp[j].cls;
				_primitives.sortedPrototypes[i] = tmp[j].proto;
				_primitives.sortedTypes[i] = tmp[j].type;
				break;
			}
		}
	}
}

Class* primitives_getClass(label_t name) {
	int low = 0, high = PRIMITIVE_NAMES_COUNT - 1;
	while (low <= high) {
		int mid = (low + high) / 2;
		if (_primitives.sortedLabels[mid] == name)
			return _primitives.sortedClasses[mid];
		if (_primitives.sortedLabels[mid] < name)
			low = mid + 1;
		else
			high = mid - 1;
	}
	return NULL;
}

Type* primitives_getType(Class* cl) {
	int low = 0, high = 17 - 1;
	while (low <= high) {
		int mid = (low + high) / 2;        
		if (_primitives.sortedClasses[mid] == cl)
			return _primitives.sortedTypes[mid];
		if (_primitives.sortedClasses[mid] < cl)
			low = mid + 1;
		else
			high = mid - 1;
	}
	return NULL;
}

Prototype* primitives_getPrototype(Class* cl) {
	int low = 0, high = 17 - 1;
	while (low <= high) {
		int mid = (low + high) / 2;        
		if (_primitives.sortedClasses[mid] == cl)
			return _primitives.sortedPrototypes[mid];
		if (_primitives.sortedClasses[mid] < cl)
			low = mid + 1;
		else
			high = mid - 1;
	}
	return NULL;
}

