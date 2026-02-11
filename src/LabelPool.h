#pragma once

#include "declarations.h"
#include "label_t.h"

#include "tools/Array.h"



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
	label_t _this;
	label_t _if;
	label_t _else;
	label_t _for;
	label_t _while;
	label_t _switch;
	
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
	label_t _size_t;

	label_t _Pointer;
	label_t _Type;
	label_t _Variadic;
	label_t _Token;

	label_t _This;
	label_t _module;
	label_t _main;
	label_t _control;
	label_t _langstd;
	label_t _token;
	label_t _fastAccess;
	label_t _stdFastAccess;
	label_t _noMeta;
	label_t _separation;
	label_t _constructor;
	label_t _argumentConstructor;
	label_t _condition;
	label_t _require;
	label_t _test;
	label_t _T;
	label_t _solitary;
	label_t _project;

	label_t _DEBUG_PRINT_CHAR;
	label_t _DEBUG_PRINT_SHORT;
	label_t _DEBUG_PRINT_INT;
	label_t _DEBUG_PRINT_LONG;
	
};


void LabelPool_create(LabelPool* pool);
void LabelPool_delete(LabelPool* pool);

label_t LabelPool_push(LabelPool* pool, const char* text);
label_t LabelPool_search(const LabelPool* pool, const char* text);


void CommonLabels_init(CommonLabels* labels, LabelPool* pool);

