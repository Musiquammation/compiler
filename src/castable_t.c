#include "castable_t.h"

#include "helper.h"

castable_t castable_cast(int srcType, int destType, castable_t value) {
	switch (destType) {
	// === unsigned char ===
	case 1:
		switch (srcType) {
			case 1: return value;
			case -1: return (castable_t){.u8 = (unsigned char)value.i8};
			case -2: return (castable_t){.u8 = (unsigned char)value.i16};
			case 2: return (castable_t){.u8 = (unsigned char)value.u16};
			case -4: return (castable_t){.u8 = (unsigned char)value.i32};
			case 4: return (castable_t){.u8 = (unsigned char)value.u32};
			case 5: return (castable_t){.u8 = (unsigned char)value.f32};
			case -8: return (castable_t){.u8 = (unsigned char)value.i64};
			case 8: return (castable_t){.u8 = (unsigned char)value.u64};
			case 9: return (castable_t){.u8 = (unsigned char)value.f64};
		}
		break;

	// === char ===
	case -1:
		switch (srcType) {
			case 1: return (castable_t){.i8 = (char)value.u8};
			case -1: return value;
			case -2: return (castable_t){.i8 = (char)value.i16};
			case 2: return (castable_t){.i8 = (char)value.u16};
			case -4: return (castable_t){.i8 = (char)value.i32};
			case 4: return (castable_t){.i8 = (char)value.u32};
			case 5: return (castable_t){.i8 = (char)value.f32};
			case -8: return (castable_t){.i8 = (char)value.i64};
			case 8: return (castable_t){.i8 = (char)value.u64};
			case 9: return (castable_t){.i8 = (char)value.f64};
		}
		break;

	// === unsigned short ===
	case 2:
		switch (srcType) {
			case 1: return (castable_t){.u16 = (unsigned short)value.u8};
			case -1: return (castable_t){.u16 = (unsigned short)value.i8};
			case 2: return value;
			case -2: return (castable_t){.u16 = (unsigned short)value.i16};
			case -4: return (castable_t){.u16 = (unsigned short)value.i32};
			case 4: return (castable_t){.u16 = (unsigned short)value.u32};
			case 5: return (castable_t){.u16 = (unsigned short)value.f32};
			case -8: return (castable_t){.u16 = (unsigned short)value.i64};
			case 8: return (castable_t){.u16 = (unsigned short)value.u64};
			case 9: return (castable_t){.u16 = (unsigned short)value.f64};
		}
		break;

	// === short ===
	case -2:
		switch (srcType) {
			case 1: return (castable_t){.i16 = (short)value.u8};
			case -1: return (castable_t){.i16 = (short)value.i8};
			case 2: return (castable_t){.i16 = (short)value.u16};
			case -2: return value;
			case -4: return (castable_t){.i16 = (short)value.i32};
			case 4: return (castable_t){.i16 = (short)value.u32};
			case 5: return (castable_t){.i16 = (short)value.f32};
			case -8: return (castable_t){.i16 = (short)value.i64};
			case 8: return (castable_t){.i16 = (short)value.u64};
			case 9: return (castable_t){.i16 = (short)value.f64};
		}
		break;

	// === unsigned int ===
	case 4:
		switch (srcType) {
			case 1: return (castable_t){.u32 = (unsigned int)value.u8};
			case -1: return (castable_t){.u32 = (unsigned int)value.i8};
			case 2: return (castable_t){.u32 = (unsigned int)value.u16};
			case -2: return (castable_t){.u32 = (unsigned int)value.i16};
			case 4: return value;
			case -4: return (castable_t){.u32 = (unsigned int)value.i32};
			case 5: return (castable_t){.u32 = (unsigned int)value.f32};
			case -8: return (castable_t){.u32 = (unsigned int)value.i64};
			case 8: return (castable_t){.u32 = (unsigned int)value.u64};
			case 9: return (castable_t){.u32 = (unsigned int)value.f64};
		}
		break;

	// === int ===
	case -4:
		switch (srcType) {
			case 1: return (castable_t){.i32 = (int)value.u8};
			case -1: return (castable_t){.i32 = (int)value.i8};
			case 2: return (castable_t){.i32 = (int)value.u16};
			case -2: return (castable_t){.i32 = (int)value.i16};
			case 4: return (castable_t){.i32 = (int)value.u32};
			case -4: return value;
			case 5: return (castable_t){.i32 = (int)value.f32};
			case -8: return (castable_t){.i32 = (int)value.i64};
			case 8: return (castable_t){.i32 = (int)value.u64};
			case 9: return (castable_t){.i32 = (int)value.f64};
		}
		break;

	// === float ===
	case 5:
		switch (srcType) {
			case 1: return (castable_t){.f32 = (float)value.u8};
			case -1: return (castable_t){.f32 = (float)value.i8};
			case 2: return (castable_t){.f32 = (float)value.u16};
			case -2: return (castable_t){.f32 = (float)value.i16};
			case 4: return (castable_t){.f32 = (float)value.u32};
			case -4: return (castable_t){.f32 = (float)value.i32};
			case 5: return value;
			case -8: return (castable_t){.f32 = (float)value.i64};
			case 8: return (castable_t){.f32 = (float)value.u64};
			case 9: return (castable_t){.f32 = (float)value.f64};
		}
		break;

	// === long long ===
	case 8:
		switch (srcType) {
			case 1: return (castable_t){.u64 = (long long)value.u8};
			case -1: return (castable_t){.u64 = (long long)value.i8};
			case 2: return (castable_t){.u64 = (long long)value.u16};
			case -2: return (castable_t){.u64 = (long long)value.i16};
			case 4: return (castable_t){.u64 = (long long)value.u32};
			case -4: return (castable_t){.u64 = (long long)value.i32};
			case 5: return (castable_t){.u64 = (long long)value.f32};
			case 8: return value;
			case -8: return (castable_t){.u64 = (long long)value.i64};
			case 9: return (castable_t){.u64 = (long long)value.f64};
		}
		break;

	// === unsigned long long ===
	case -8:
		switch (srcType) {
			case 1: return (castable_t){.i64 = (unsigned long long)value.u8};
			case -1: return (castable_t){.i64 = (unsigned long long)value.i8};
			case 2: return (castable_t){.i64 = (unsigned long long)value.u16};
			case -2: return (castable_t){.i64 = (unsigned long long)value.i16};
			case 4: return (castable_t){.i64 = (unsigned long long)value.u32};
			case -4: return (castable_t){.i64 = (unsigned long long)value.i32};
			case 5: return (castable_t){.i64 = (unsigned long long)value.f32};
			case 8: return (castable_t){.i64 = (unsigned long long)value.u64};
			case -8: return value;
			case 9: return (castable_t){.i64 = (unsigned long long)value.f64};
		}
		break;

	// === double ===
	case 9:
		switch (srcType) {
			case 1: return (castable_t){.i64 = (double)value.u8};
			case -1: return (castable_t){.i64 = (double)value.i8};
			case 2: return (castable_t){.i64 = (double)value.u16};
			case -2: return (castable_t){.i64 = (double)value.i16};
			case 4: return (castable_t){.i64 = (double)value.u32};
			case -4: return (castable_t){.i64 = (double)value.i32};
			case 5: return (castable_t){.i64 = (double)value.f32};
			case 8: return (castable_t){.i64 = (double)value.u64};
			case -8: return (castable_t){.i64 = (double)value.i64};
			case 9: return value;
		}
		break;
	}


	raiseError("[Memory] Invalid cast request");
}
