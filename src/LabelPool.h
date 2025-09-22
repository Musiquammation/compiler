#ifndef COMPILER_LABEL_POOL_H_
#define COMPILER_LABEL_POOL_H_

#include "declarations.h"
#include "label_t.h"

#include <tools/Array.h>



struct LabelPool {
	char** data;
	int size;
	int capacity;
};


struct CommonLabels {
	label_t _let;
	label_t _const;
	label_t _class;
	label_t _function;
	label_t _if;
	label_t _else;
	label_t _for;
	label_t _while;
	label_t _switch;
	
	label_t _module;
	label_t _main;
	
	label_t _i8;
	label_t _u8;
	label_t _i16;
	label_t _u16;
	label_t _i32;
	label_t _u32;
	label_t _i64;
	label_t _u64;
	label_t _f32;
	label_t _f64;

	label_t _char;
	label_t _bool;
	label_t _short;
	label_t _int;
	label_t _long;
	label_t _float;
	label_t _double;
};


void LabelPool_create(LabelPool* pool);
void LabelPool_delete(LabelPool* pool);

label_t LabelPool_push(LabelPool* pool, const char* text);
label_t LabelPool_search(const LabelPool* pool, const char* text);


void CommonLabels_generate(CommonLabels* labels, LabelPool* pool);

#endif
