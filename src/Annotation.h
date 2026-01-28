#pragma once

#include "label_t.h"
struct Annotation {
	int type;

	union {
		struct {
			int stdBehavior;
		} stdFastAccess;
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
	ANNOTATION_CONSTRUCTOR
};