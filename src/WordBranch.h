#ifndef COMPILER_WORDBRANCH_H_
#define COMPILER_WORDBRANCH_H_

#include "declarations.h"

enum {
	WORDBRANCH_VARIABLE
};

struct WordBranch {
	Variable* v;
	int type;
};

struct WordBranchNode {
	WordBranchNode* next;
	WordBranch branch;
};

#endif