#ifndef TOOLS_TOOLS_H_
#define TOOLS_TOOLS_H_

#include <stdbool.h>

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

#define structdef(name) typedef struct name name

#define NULL_UINT ((uint)-1)
#define NULL_USHORT ((ushort)-1)


#define get_array_nth_bit(array, p) ((((array)[p>>3]) >> (p%8)) & 1)

#endif