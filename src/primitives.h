#ifndef COMPILER_PRIMITIVES_H_
#define COMPILER_PRIMITIVES_H_

#include "declarations.h"

#include "Class.h"
#include "Type.h"

typedef struct {
	Class class_i8;
	Class class_u8;
	Class class_i16;
	Class class_u16;
	Class class_i32;
	Class class_u32;
	Class class_i64;
	Class class_u64;
	Class class_f32;
	Class class_f64;

	Type type_i8;
	Type type_u8;
	Type type_i16;
	Type type_u16;
	Type type_i32;
	Type type_u32;
	Type type_i64;
	Type type_u64;
	Type type_f32;
	Type type_f64;

	label_t sortedLabels[17];
	Class* sortedClasses[17];
	Type* sortedTypes[17];

} PrimitiveContainer;

extern PrimitiveContainer _primitives;





void primitives_init(void);
Class* primitives_getClass(label_t name);
Type* primitives_getType(Class* cl);

#endif