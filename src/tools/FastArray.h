#ifndef TOOLS_FASTARRAY_H_
#define TOOLS_FASTARRAY_H_

#include "tools.h"

structdef(FastArray);

struct FastArray {
	void* data;
	ushort length;
};



#endif