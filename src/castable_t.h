#ifndef COMPILER_CASTABLE_T_H_
#define COMPILER_CASTABLE_T_H_

typedef union {
	char i8;
	unsigned char u8;

	short i16;
	unsigned short u16;

	int i32;
	unsigned int u32;

	float f32;
	double f64;

	long i64;
	unsigned long u64;
} castable_t;




#endif