#include "Expression.h"
#include "helper.h"

int Expression_simplify(
	int type,
	int type_op1,
	int type_op2,
	Expression* target,
	const Expression* left,
	const Expression* right
) {
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
	} value_t;

	value_t leftValue;
	value_t rightValue;

	int finalType;



	#define copyValue(dst, src, type) \
		switch(type) { \
			case EXPRESSION_I8:  dst.i8  = src.i8;  break; \
			case EXPRESSION_U8:  dst.u8  = src.u8;  break; \
			case EXPRESSION_I16: dst.i16 = src.i16; break; \
			case EXPRESSION_U16: dst.u16 = src.u16; break; \
			case EXPRESSION_I32: dst.i32 = src.i32; break; \
			case EXPRESSION_U32: dst.u32 = src.u32; break; \
			case EXPRESSION_I64: dst.i64 = src.i64; break; \
			case EXPRESSION_U64: dst.u64 = src.u64; break; \
			case EXPRESSION_F32: dst.f32 = src.f32; break; \
			case EXPRESSION_F64: dst.f64 = src.f64; break; \
			default: /* erreur */ break; \
		}

	if (type_op2 == -1) {
		copyValue(leftValue, left->data, type_op1);
		finalType = type_op1;

	} else if (type_op1 < type_op2) {
		finalType = type_op2;
		switch (type_op2) {
			case EXPRESSION_U8:
				rightValue.u8 = right->data.u8;
				break;

			case EXPRESSION_I8:
				rightValue.i8 = right->data.i8;
				switch (type_op1) {
					case EXPRESSION_U8: leftValue.i8 = (char)left->data.u8; break;
				}
				break;

			case EXPRESSION_U16:
				rightValue.u16 = right->data.u16;
				switch (type_op1) {
					case EXPRESSION_U8: leftValue.u16 = (unsigned short)left->data.u8; break;
					case EXPRESSION_I8: leftValue.u16 = (unsigned short)left->data.i8; break;
				}
				break;

			case EXPRESSION_I16:
				rightValue.i16 = right->data.i16;
				switch (type_op1) {
					case EXPRESSION_U8: leftValue.i16 = (short)left->data.u8; break;
					case EXPRESSION_I8: leftValue.i16 = (short)left->data.i8; break;
					case EXPRESSION_U16: leftValue.i16 = (short)left->data.u16; break;
				}
				break;

			case EXPRESSION_U32:
				rightValue.u32 = right->data.u32;
				switch (type_op1) {
					case EXPRESSION_U8: leftValue.u32 = (unsigned int)left->data.u8; break;
					case EXPRESSION_I8: leftValue.u32 = (unsigned int)left->data.i8; break;
					case EXPRESSION_U16: leftValue.u32 = (unsigned int)left->data.u16; break;
					case EXPRESSION_I16: leftValue.u32 = (unsigned int)left->data.i16; break;
				}
				break;

			case EXPRESSION_I32:
				rightValue.i32 = right->data.i32;
				switch (type_op1) {
					case EXPRESSION_U8: leftValue.i32 = (int)left->data.u8; break;
					case EXPRESSION_I8: leftValue.i32 = (int)left->data.i8; break;
					case EXPRESSION_U16: leftValue.i32 = (int)left->data.u16; break;
					case EXPRESSION_I16: leftValue.i32 = (int)left->data.i16; break;
					case EXPRESSION_U32: leftValue.i32 = (int)left->data.u32; break;
				}
				break;

			case EXPRESSION_F32:
				rightValue.f32 = right->data.f32;
				switch (type_op1) {
					case EXPRESSION_U8: leftValue.f32 = (float)left->data.u8; break;
					case EXPRESSION_I8: leftValue.f32 = (float)left->data.i8; break;
					case EXPRESSION_U16: leftValue.f32 = (float)left->data.u16; break;
					case EXPRESSION_I16: leftValue.f32 = (float)left->data.i16; break;
					case EXPRESSION_U32: leftValue.f32 = (float)left->data.u32; break;
					case EXPRESSION_I32: leftValue.f32 = (float)left->data.i32; break;
				}
				break;

			case EXPRESSION_U64:
				rightValue.u64 = right->data.u64;
				switch (type_op1) {
					case EXPRESSION_U8: leftValue.u64 = (unsigned long long)left->data.u8; break;
					case EXPRESSION_I8: leftValue.u64 = (unsigned long long)left->data.i8; break;
					case EXPRESSION_U16: leftValue.u64 = (unsigned long long)left->data.u16; break;
					case EXPRESSION_I16: leftValue.u64 = (unsigned long long)left->data.i16; break;
					case EXPRESSION_U32: leftValue.u64 = (unsigned long long)left->data.u32; break;
					case EXPRESSION_I32: leftValue.u64 = (unsigned long long)left->data.i32; break;
					case EXPRESSION_F32: leftValue.u64 = (unsigned long long)left->data.f32; break;
				}
				break;

			case EXPRESSION_I64:
				rightValue.i64 = right->data.i64;
				switch (type_op1) {
					case EXPRESSION_U8: leftValue.i64 = (long long)left->data.u8; break;
					case EXPRESSION_I8: leftValue.i64 = (long long)left->data.i8; break;
					case EXPRESSION_U16: leftValue.i64 = (long long)left->data.u16; break;
					case EXPRESSION_I16: leftValue.i64 = (long long)left->data.i16; break;
					case EXPRESSION_U32: leftValue.i64 = (long long)left->data.u32; break;
					case EXPRESSION_I32: leftValue.i64 = (long long)left->data.i32; break;
					case EXPRESSION_U64: leftValue.i64 = (long long)left->data.u64; break;
				}
				break;

			case EXPRESSION_F64:
				rightValue.f64 = left->data.f64;
				switch (type_op1) {
					case EXPRESSION_U8: leftValue.f64 = (double)left->data.u8; break;
					case EXPRESSION_I8: leftValue.f64 = (double)left->data.i8; break;
					case EXPRESSION_U16: leftValue.f64 = (double)left->data.u16; break;
					case EXPRESSION_I16: leftValue.f64 = (double)left->data.i16; break;
					case EXPRESSION_U32: leftValue.f64 = (double)left->data.u32; break;
					case EXPRESSION_I32: leftValue.f64 = (double)left->data.i32; break;
					case EXPRESSION_U64: leftValue.f64 = (double)left->data.u64; break;
					case EXPRESSION_I64: leftValue.f64 = (double)left->data.i64; break;
					case EXPRESSION_F32: leftValue.f64 = (double)left->data.f32; break;
				}
				break;
		}

	} else if (type_op1 > type_op2) {
		finalType = type_op1;  // cette fois, le type final est le type le plus grand (type_op1)
		switch (type_op1) {
		case EXPRESSION_U8:
			leftValue.u8 = left->data.u8;
			break;

		case EXPRESSION_I8:
			leftValue.i8 = left->data.i8;
			switch (type_op2) {
				case EXPRESSION_U8: rightValue.i8 = (char)right->data.u8; break;
			}
			break;

		case EXPRESSION_U16:
			leftValue.u16 = left->data.u16;
			switch (type_op2) {
				case EXPRESSION_U8: rightValue.u16 = (unsigned short)right->data.u8; break;
				case EXPRESSION_I8: rightValue.u16 = (unsigned short)right->data.i8; break;
			}
			break;

		case EXPRESSION_I16:
			leftValue.i16 = left->data.i16;
			switch (type_op2) {
				case EXPRESSION_U8: rightValue.i16 = (short)right->data.u8; break;
				case EXPRESSION_I8: rightValue.i16 = (short)right->data.i8; break;
				case EXPRESSION_U16: rightValue.i16 = (short)right->data.u16; break;
			}
			break;

		case EXPRESSION_U32:
			leftValue.u32 = left->data.u32;
			switch (type_op2) {
				case EXPRESSION_U8: rightValue.u32 = (unsigned int)right->data.u8; break;
				case EXPRESSION_I8: rightValue.u32 = (unsigned int)right->data.i8; break;
				case EXPRESSION_U16: rightValue.u32 = (unsigned int)right->data.u16; break;
				case EXPRESSION_I16: rightValue.u32 = (unsigned int)right->data.i16; break;
			}
			break;

		case EXPRESSION_I32:
			leftValue.i32 = left->data.i32;
			switch (type_op2) {
				case EXPRESSION_U8: rightValue.i32 = (int)right->data.u8; break;
				case EXPRESSION_I8: rightValue.i32 = (int)right->data.i8; break;
				case EXPRESSION_U16: rightValue.i32 = (int)right->data.u16; break;
				case EXPRESSION_I16: rightValue.i32 = (int)right->data.i16; break;
				case EXPRESSION_U32: rightValue.i32 = (int)right->data.u32; break;
			}
			break;

		case EXPRESSION_F32:
			leftValue.f32 = left->data.f32;
			switch (type_op2) {
				case EXPRESSION_U8: rightValue.f32 = (float)right->data.u8; break;
				case EXPRESSION_I8: rightValue.f32 = (float)right->data.i8; break;
				case EXPRESSION_U16: rightValue.f32 = (float)right->data.u16; break;
				case EXPRESSION_I16: rightValue.f32 = (float)right->data.i16; break;
				case EXPRESSION_U32: rightValue.f32 = (float)right->data.u32; break;
				case EXPRESSION_I32: rightValue.f32 = (float)right->data.i32; break;
			}
			break;

		case EXPRESSION_U64:
			leftValue.u64 = left->data.u64;
			switch (type_op2) {
				case EXPRESSION_U8: rightValue.u64 = (unsigned long long)right->data.u8; break;
				case EXPRESSION_I8: rightValue.u64 = (unsigned long long)right->data.i8; break;
				case EXPRESSION_U16: rightValue.u64 = (unsigned long long)right->data.u16; break;
				case EXPRESSION_I16: rightValue.u64 = (unsigned long long)right->data.i16; break;
				case EXPRESSION_U32: rightValue.u64 = (unsigned long long)right->data.u32; break;
				case EXPRESSION_I32: rightValue.u64 = (unsigned long long)right->data.i32; break;
				case EXPRESSION_F32: rightValue.u64 = (unsigned long long)right->data.f32; break;
			}
			break;

		case EXPRESSION_I64:
			leftValue.i64 = left->data.i64;
			switch (type_op2) {
				case EXPRESSION_U8: rightValue.i64 = (long long)right->data.u8; break;
				case EXPRESSION_I8: rightValue.i64 = (long long)right->data.i8; break;
				case EXPRESSION_U16: rightValue.i64 = (long long)right->data.u16; break;
				case EXPRESSION_I16: rightValue.i64 = (long long)right->data.i16; break;
				case EXPRESSION_U32: rightValue.i64 = (long long)right->data.u32; break;
				case EXPRESSION_I32: rightValue.i64 = (long long)right->data.i32; break;
				case EXPRESSION_U64: rightValue.i64 = (long long)right->data.u64; break;
			}
			break;

		case EXPRESSION_F64:
			leftValue.f64 = left->data.f64;
			switch (type_op2) {
				case EXPRESSION_U8: rightValue.f64 = (double)right->data.u8; break;
				case EXPRESSION_I8: rightValue.f64 = (double)right->data.i8; break;
				case EXPRESSION_U16: rightValue.f64 = (double)right->data.u16; break;
				case EXPRESSION_I16: rightValue.f64 = (double)right->data.i16; break;
				case EXPRESSION_U32: rightValue.f64 = (double)right->data.u32; break;
				case EXPRESSION_I32: rightValue.f64 = (double)right->data.i32; break;
				case EXPRESSION_U64: rightValue.f64 = (double)right->data.u64; break;
				case EXPRESSION_I64: rightValue.f64 = (double)right->data.i64; break;
				case EXPRESSION_F32: rightValue.f64 = (double)right->data.f32; break;
			}
			break;
		}

		
	} else {
		copyValue(leftValue, left->data, type_op1);
		copyValue(rightValue, right->data, type_op2);
		finalType = type_op1;
	}



	#define scale(type, op) (((type-EXPRESSION_ADDITION) << 6) | (op - EXPRESSION_U8))

	switch (scale(type, finalType)) {

    // -------------------- ADDITION --------------------
    case scale(EXPRESSION_ADDITION, EXPRESSION_U8):   target->data.u8  = leftValue.u8  + rightValue.u8;  break;
    case scale(EXPRESSION_ADDITION, EXPRESSION_I8):   target->data.i8  = leftValue.i8  + rightValue.i8;  break;
    case scale(EXPRESSION_ADDITION, EXPRESSION_U16):  target->data.u16 = leftValue.u16 + rightValue.u16; break;
    case scale(EXPRESSION_ADDITION, EXPRESSION_I16):  target->data.i16 = leftValue.i16 + rightValue.i16; break;
    case scale(EXPRESSION_ADDITION, EXPRESSION_U32):  target->data.u32 = leftValue.u32 + rightValue.u32; break;
    case scale(EXPRESSION_ADDITION, EXPRESSION_I32):  target->data.i32 = leftValue.i32 + rightValue.i32; break;
    case scale(EXPRESSION_ADDITION, EXPRESSION_F32):  target->data.f32 = leftValue.f32 + rightValue.f32; break;
    case scale(EXPRESSION_ADDITION, EXPRESSION_U64):  target->data.u64 = leftValue.u64 + rightValue.u64; break;
    case scale(EXPRESSION_ADDITION, EXPRESSION_I64):  target->data.i64 = leftValue.i64 + rightValue.i64; break;
    case scale(EXPRESSION_ADDITION, EXPRESSION_F64):  target->data.f64 = leftValue.f64 + rightValue.f64; break;

    // -------------------- SUBTRACTION --------------------
    case scale(EXPRESSION_SUBSTRACTION, EXPRESSION_U8):   target->data.u8  = leftValue.u8  - rightValue.u8;  break;
    case scale(EXPRESSION_SUBSTRACTION, EXPRESSION_I8):   target->data.i8  = leftValue.i8  - rightValue.i8;  break;
    case scale(EXPRESSION_SUBSTRACTION, EXPRESSION_U16):  target->data.u16 = leftValue.u16 - rightValue.u16; break;
    case scale(EXPRESSION_SUBSTRACTION, EXPRESSION_I16):  target->data.i16 = leftValue.i16 - rightValue.i16; break;
    case scale(EXPRESSION_SUBSTRACTION, EXPRESSION_U32):  target->data.u32 = leftValue.u32 - rightValue.u32; break;
    case scale(EXPRESSION_SUBSTRACTION, EXPRESSION_I32):  target->data.i32 = leftValue.i32 - rightValue.i32; break;
    case scale(EXPRESSION_SUBSTRACTION, EXPRESSION_F32):  target->data.f32 = leftValue.f32 - rightValue.f32; break;
    case scale(EXPRESSION_SUBSTRACTION, EXPRESSION_U64):  target->data.u64 = leftValue.u64 - rightValue.u64; break;
    case scale(EXPRESSION_SUBSTRACTION, EXPRESSION_I64):  target->data.i64 = leftValue.i64 - rightValue.i64; break;
    case scale(EXPRESSION_SUBSTRACTION, EXPRESSION_F64):  target->data.f64 = leftValue.f64 - rightValue.f64; break;

    // -------------------- MULTIPLICATION --------------------
    case scale(EXPRESSION_MULTIPLICATION, EXPRESSION_U8):   target->data.u8  = leftValue.u8  * rightValue.u8;  break;
    case scale(EXPRESSION_MULTIPLICATION, EXPRESSION_I8):   target->data.i8  = leftValue.i8  * rightValue.i8;  break;
    case scale(EXPRESSION_MULTIPLICATION, EXPRESSION_U16):  target->data.u16 = leftValue.u16 * rightValue.u16; break;
    case scale(EXPRESSION_MULTIPLICATION, EXPRESSION_I16):  target->data.i16 = leftValue.i16 * rightValue.i16; break;
    case scale(EXPRESSION_MULTIPLICATION, EXPRESSION_U32):  target->data.u32 = leftValue.u32 * rightValue.u32; break;
    case scale(EXPRESSION_MULTIPLICATION, EXPRESSION_I32):  target->data.i32 = leftValue.i32 * rightValue.i32; break;
    case scale(EXPRESSION_MULTIPLICATION, EXPRESSION_F32):  target->data.f32 = leftValue.f32 * rightValue.f32; break;
    case scale(EXPRESSION_MULTIPLICATION, EXPRESSION_U64):  target->data.u64 = leftValue.u64 * rightValue.u64; break;
    case scale(EXPRESSION_MULTIPLICATION, EXPRESSION_I64):  target->data.i64 = leftValue.i64 * rightValue.i64; break;
    case scale(EXPRESSION_MULTIPLICATION, EXPRESSION_F64):  target->data.f64 = leftValue.f64 * rightValue.f64; break;

    // -------------------- DIVISION --------------------
    case scale(EXPRESSION_DIVISION, EXPRESSION_U8):   target->data.u8  = leftValue.u8  / rightValue.u8;  break;
    case scale(EXPRESSION_DIVISION, EXPRESSION_I8):   target->data.i8  = leftValue.i8  / rightValue.i8;  break;
    case scale(EXPRESSION_DIVISION, EXPRESSION_U16):  target->data.u16 = leftValue.u16 / rightValue.u16; break;
    case scale(EXPRESSION_DIVISION, EXPRESSION_I16):  target->data.i16 = leftValue.i16 / rightValue.i16; break;
    case scale(EXPRESSION_DIVISION, EXPRESSION_U32):  target->data.u32 = leftValue.u32 / rightValue.u32; break;
    case scale(EXPRESSION_DIVISION, EXPRESSION_I32):  target->data.i32 = leftValue.i32 / rightValue.i32; break;
    case scale(EXPRESSION_DIVISION, EXPRESSION_F32):  target->data.f32 = leftValue.f32 / rightValue.f32; break;
    case scale(EXPRESSION_DIVISION, EXPRESSION_U64):  target->data.u64 = leftValue.u64 / rightValue.u64; break;
    case scale(EXPRESSION_DIVISION, EXPRESSION_I64):  target->data.i64 = leftValue.i64 / rightValue.i64; break;
    case scale(EXPRESSION_DIVISION, EXPRESSION_F64):  target->data.f64 = leftValue.f64 / rightValue.f64; break;

    // -------------------- MODULO (entiers uniquement) --------------------
    case scale(EXPRESSION_MODULO, EXPRESSION_U8):   target->data.u8  = leftValue.u8  % rightValue.u8;  break;
    case scale(EXPRESSION_MODULO, EXPRESSION_I8):   target->data.i8  = leftValue.i8  % rightValue.i8;  break;
    case scale(EXPRESSION_MODULO, EXPRESSION_U16):  target->data.u16 = leftValue.u16 % rightValue.u16; break;
    case scale(EXPRESSION_MODULO, EXPRESSION_I16):  target->data.i16 = leftValue.i16 % rightValue.i16; break;
    case scale(EXPRESSION_MODULO, EXPRESSION_U32):  target->data.u32 = leftValue.u32 % rightValue.u32; break;
    case scale(EXPRESSION_MODULO, EXPRESSION_I32):  target->data.i32 = leftValue.i32 % rightValue.i32; break;
    case scale(EXPRESSION_MODULO, EXPRESSION_U64):  target->data.u64 = leftValue.u64 % rightValue.u64; break;
    case scale(EXPRESSION_MODULO, EXPRESSION_I64):  target->data.i64 = leftValue.i64 % rightValue.i64; break;

	// -------------------- BITWISE AND --------------------
	case scale(EXPRESSION_BITWISE_AND, EXPRESSION_U8):   target->data.u8  = leftValue.u8  & rightValue.u8;  break;
	case scale(EXPRESSION_BITWISE_AND, EXPRESSION_I8):   target->data.i8  = leftValue.i8  & rightValue.i8;  break;
	case scale(EXPRESSION_BITWISE_AND, EXPRESSION_U16):  target->data.u16 = leftValue.u16 & rightValue.u16; break;
	case scale(EXPRESSION_BITWISE_AND, EXPRESSION_I16):  target->data.i16 = leftValue.i16 & rightValue.i16; break;
	case scale(EXPRESSION_BITWISE_AND, EXPRESSION_U32):  target->data.u32 = leftValue.u32 & rightValue.u32; break;
	case scale(EXPRESSION_BITWISE_AND, EXPRESSION_I32):  target->data.i32 = leftValue.i32 & rightValue.i32; break;
	case scale(EXPRESSION_BITWISE_AND, EXPRESSION_U64):  target->data.u64 = leftValue.u64 & rightValue.u64; break;
	case scale(EXPRESSION_BITWISE_AND, EXPRESSION_I64):  target->data.i64 = leftValue.i64 & rightValue.i64; break;

	// -------------------- BITWISE OR --------------------
	case scale(EXPRESSION_BITWISE_OR, EXPRESSION_U8):   target->data.u8  = leftValue.u8  | rightValue.u8;  break;
	case scale(EXPRESSION_BITWISE_OR, EXPRESSION_I8):   target->data.i8  = leftValue.i8  | rightValue.i8;  break;
	case scale(EXPRESSION_BITWISE_OR, EXPRESSION_U16):  target->data.u16 = leftValue.u16 | rightValue.u16; break;
	case scale(EXPRESSION_BITWISE_OR, EXPRESSION_I16):  target->data.i16 = leftValue.i16 | rightValue.i16; break;
	case scale(EXPRESSION_BITWISE_OR, EXPRESSION_U32):  target->data.u32 = leftValue.u32 | rightValue.u32; break;
	case scale(EXPRESSION_BITWISE_OR, EXPRESSION_I32):  target->data.i32 = leftValue.i32 | rightValue.i32; break;
	case scale(EXPRESSION_BITWISE_OR, EXPRESSION_U64):  target->data.u64 = leftValue.u64 | rightValue.u64; break;
	case scale(EXPRESSION_BITWISE_OR, EXPRESSION_I64):  target->data.i64 = leftValue.i64 | rightValue.i64; break;

	// -------------------- BITWISE XOR --------------------
	case scale(EXPRESSION_BITWISE_XOR, EXPRESSION_U8):   target->data.u8  = leftValue.u8  ^ rightValue.u8;  break;
	case scale(EXPRESSION_BITWISE_XOR, EXPRESSION_I8):   target->data.i8  = leftValue.i8  ^ rightValue.i8;  break;
	case scale(EXPRESSION_BITWISE_XOR, EXPRESSION_U16):  target->data.u16 = leftValue.u16 ^ rightValue.u16; break;
	case scale(EXPRESSION_BITWISE_XOR, EXPRESSION_I16):  target->data.i16 = leftValue.i16 ^ rightValue.i16; break;
	case scale(EXPRESSION_BITWISE_XOR, EXPRESSION_U32):  target->data.u32 = leftValue.u32 ^ rightValue.u32; break;
	case scale(EXPRESSION_BITWISE_XOR, EXPRESSION_I32):  target->data.i32 = leftValue.i32 ^ rightValue.i32; break;
	case scale(EXPRESSION_BITWISE_XOR, EXPRESSION_U64):  target->data.u64 = leftValue.u64 ^ rightValue.u64; break;
	case scale(EXPRESSION_BITWISE_XOR, EXPRESSION_I64):  target->data.i64 = leftValue.i64 ^ rightValue.i64; break;

	// -------------------- LEFT SHIFT --------------------
	case scale(EXPRESSION_LEFT_SHIFT, EXPRESSION_U8):   target->data.u8  = leftValue.u8  << rightValue.u8;  break;
	case scale(EXPRESSION_LEFT_SHIFT, EXPRESSION_I8):   target->data.i8  = leftValue.i8  << rightValue.i8;  break;
	case scale(EXPRESSION_LEFT_SHIFT, EXPRESSION_U16):  target->data.u16 = leftValue.u16 << rightValue.u16; break;
	case scale(EXPRESSION_LEFT_SHIFT, EXPRESSION_I16):  target->data.i16 = leftValue.i16 << rightValue.i16; break;
	case scale(EXPRESSION_LEFT_SHIFT, EXPRESSION_U32):  target->data.u32 = leftValue.u32 << rightValue.u32; break;
	case scale(EXPRESSION_LEFT_SHIFT, EXPRESSION_I32):  target->data.i32 = leftValue.i32 << rightValue.i32; break;
	case scale(EXPRESSION_LEFT_SHIFT, EXPRESSION_U64):  target->data.u64 = leftValue.u64 << rightValue.u64; break;
	case scale(EXPRESSION_LEFT_SHIFT, EXPRESSION_I64):  target->data.i64 = leftValue.i64 << rightValue.i64; break;

	// -------------------- RIGHT SHIFT --------------------
	case scale(EXPRESSION_RIGHT_SHIFT, EXPRESSION_U8):   target->data.u8  = leftValue.u8  >> rightValue.u8;  break;
	case scale(EXPRESSION_RIGHT_SHIFT, EXPRESSION_I8):   target->data.i8  = leftValue.i8  >> rightValue.i8;  break;
	case scale(EXPRESSION_RIGHT_SHIFT, EXPRESSION_U16):  target->data.u16 = leftValue.u16 >> rightValue.u16; break;
	case scale(EXPRESSION_RIGHT_SHIFT, EXPRESSION_I16):  target->data.i16 = leftValue.i16 >> rightValue.i16; break;
	case scale(EXPRESSION_RIGHT_SHIFT, EXPRESSION_U32):  target->data.u32 = leftValue.u32 >> rightValue.u32; break;
	case scale(EXPRESSION_RIGHT_SHIFT, EXPRESSION_I32):  target->data.i32 = leftValue.i32 >> rightValue.i32; break;
	case scale(EXPRESSION_RIGHT_SHIFT, EXPRESSION_U64):  target->data.u64 = leftValue.u64 >> rightValue.u64; break;
	case scale(EXPRESSION_RIGHT_SHIFT, EXPRESSION_I64):  target->data.i64 = leftValue.i64 >> rightValue.i64; break;

    // -------------------- UNARY OPERATORS --------------------
    case scale(EXPRESSION_BITWISE_NOT, EXPRESSION_U8):     target->data.u8  = ~leftValue.u8; break;
    case scale(EXPRESSION_BITWISE_NOT, EXPRESSION_I8):     target->data.i8  = ~leftValue.i8; break;
    case scale(EXPRESSION_BITWISE_NOT, EXPRESSION_U16):    target->data.u16 = ~leftValue.u16; break;
    case scale(EXPRESSION_BITWISE_NOT, EXPRESSION_I16):    target->data.i16 = ~leftValue.i16; break;
    case scale(EXPRESSION_BITWISE_NOT, EXPRESSION_U32):    target->data.u32 = ~leftValue.u32; break;
    case scale(EXPRESSION_BITWISE_NOT, EXPRESSION_I32):    target->data.i32 = ~leftValue.i32; break;
    case scale(EXPRESSION_BITWISE_NOT, EXPRESSION_U64):    target->data.u64 = ~leftValue.u64; break;
    case scale(EXPRESSION_BITWISE_NOT, EXPRESSION_I64):    target->data.i64 = ~leftValue.i64; break;

    case scale(EXPRESSION_LOGICAL_NOT, EXPRESSION_U8):     target->data.u8  = !leftValue.u8; break;
    case scale(EXPRESSION_LOGICAL_NOT, EXPRESSION_I8):     target->data.i8  = !leftValue.i8; break;
    case scale(EXPRESSION_LOGICAL_NOT, EXPRESSION_U16):    target->data.u16 = !leftValue.u16; break;
    case scale(EXPRESSION_LOGICAL_NOT, EXPRESSION_I16):    target->data.i16 = !leftValue.i16; break;
    case scale(EXPRESSION_LOGICAL_NOT, EXPRESSION_U32):    target->data.u32 = !leftValue.u32; break;
    case scale(EXPRESSION_LOGICAL_NOT, EXPRESSION_I32):    target->data.i32 = !leftValue.i32; break;
    case scale(EXPRESSION_LOGICAL_NOT, EXPRESSION_U64):    target->data.u64 = !leftValue.u64; break;
    case scale(EXPRESSION_LOGICAL_NOT, EXPRESSION_I64):    target->data.i64 = !leftValue.i64; break;

    case scale(EXPRESSION_POSITIVE, EXPRESSION_U8):        target->data.u8  = +leftValue.u8; break;
    case scale(EXPRESSION_POSITIVE, EXPRESSION_I8):        target->data.i8  = +leftValue.i8; break;
    case scale(EXPRESSION_POSITIVE, EXPRESSION_U16):       target->data.u16 = +leftValue.u16; break;
    case scale(EXPRESSION_POSITIVE, EXPRESSION_I16):       target->data.i16 = +leftValue.i16; break;
    case scale(EXPRESSION_POSITIVE, EXPRESSION_U32):       target->data.u32 = +leftValue.u32; break;
    case scale(EXPRESSION_POSITIVE, EXPRESSION_I32):       target->data.i32 = +leftValue.i32; break;
    case scale(EXPRESSION_POSITIVE, EXPRESSION_U64):       target->data.u64 = +leftValue.u64; break;
    case scale(EXPRESSION_POSITIVE, EXPRESSION_I64):       target->data.i64 = +leftValue.i64; break;
    case scale(EXPRESSION_POSITIVE, EXPRESSION_F32):       target->data.f32 = +leftValue.f32; break;
    case scale(EXPRESSION_POSITIVE, EXPRESSION_F64):       target->data.f64 = +leftValue.f64; break;

    case scale(EXPRESSION_NEGATION, EXPRESSION_U8):        target->data.u8  = -leftValue.u8; break;
    case scale(EXPRESSION_NEGATION, EXPRESSION_I8):        target->data.i8  = -leftValue.i8; break;
    case scale(EXPRESSION_NEGATION, EXPRESSION_U16):       target->data.u16 = -leftValue.u16; break;
    case scale(EXPRESSION_NEGATION, EXPRESSION_I16):       target->data.i16 = -leftValue.i16; break;
    case scale(EXPRESSION_NEGATION, EXPRESSION_U32):       target->data.u32 = -leftValue.u32; break;
    case scale(EXPRESSION_NEGATION, EXPRESSION_I32):       target->data.i32 = -leftValue.i32; break;
    case scale(EXPRESSION_NEGATION, EXPRESSION_U64):       target->data.u64 = -leftValue.u64; break;
    case scale(EXPRESSION_NEGATION, EXPRESSION_I64):       target->data.i64 = -leftValue.i64; break;
    case scale(EXPRESSION_NEGATION, EXPRESSION_F32):       target->data.f32 = -leftValue.f32; break;
    case scale(EXPRESSION_NEGATION, EXPRESSION_F64):       target->data.f64 = -leftValue.f64; break;

    // -------------------- DEFAULT --------------------
    default:
        raiseError("Invalid operation");
        break;
}




	#undef scale

	return finalType;

}