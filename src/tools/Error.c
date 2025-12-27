#include "Error.h"

uint Error_try(jmp_buf handler, uint(*run)()) {
	const int value = setjmp(handler);
	if (value == 0)
		return (*run)();
	
	return value;
}


uint Error_try_catch(jmp_buf handler, uint(*run)(), uint(*error)(uint)) {
	const int value = setjmp(handler);
	if (value == 0)
		return (*run)();
	
	return (*error)(value);
}

void Error_throw(jmp_buf handler, uint error) {
	longjmp(handler, error == 0 ? NULL_UINT : error);
}

