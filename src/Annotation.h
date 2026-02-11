#pragma once

#include "label_t.h"
#include "declarations.h"

struct Annotation {
	int type;

	union {
		struct {
			int stdBehavior;
		} stdFastAccess;

		struct {
			label_t pass;
			label_t miss;
		} condition;

		struct {
			label_t pass;
			label_t miss;
		} test;

		struct {
			label_t object;
			label_t fn;
		} require;

		struct {
			label_t name;
			Prototype* proto;
		} project;
	};
};


enum {
	ANNOTATION_MAIN,
	ANNOTATION_CONTROL,
	ANNOTATION_LANGSTD,
	ANNOTATION_FAST_ACCESS,
	ANNOTATION_STD_FAST_ACCESS,
	ANNOTATION_SEPARATION,
	ANNOTATION_CONSTRUCTOR,
	ANNOTATION_ARGUMENT_CONSTRUCTOR,
	ANNOTATION_CONDITION,
	ANNOTATION_TEST,
	ANNOTATION_REQUIRE,
	ANNOTATION_PROJECT,
	ANNOTATION_SOLITARY,
};


void Annotation_fillProjection(FunctionArgProjection* proj, Annotation* annotation);