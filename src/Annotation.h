#pragma once

#include "label_t.h"
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
	};
};


enum {
	ANNOTATION_MAIN,
	ANNOTATION_CONTROL,
	ANNOTATION_LANGSTD,
	ANNOTATION_FAST_ACCESS,
	ANNOTATION_STD_FAST_ACCESS,
	ANNOTATION_NO_META,
	ANNOTATION_SEPARATION,
	ANNOTATION_CONSTRUCTOR,
	ANNOTATION_ARGUMENT_CONSTRUCTOR,
	ANNOTATION_CONDITION,
	ANNOTATION_TEST,
	ANNOTATION_REQUIRE,
};