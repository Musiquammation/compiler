#include "chooseSign.h"

#include "Expression.h"
#include "castable_t.h"

int chooseSign(int signedLeft, int signedRight) {
	int left = signedLeft >= 0 ? signedLeft : -signedLeft;
	int right = signedRight >= 0 ? signedRight : -signedRight;

	if (left == EXPRSIZE_FLOATING) {
		if (right == CASTABLE_FLOAT) {
			return CASTABLE_FLOAT;
		}

		if (right == CASTABLE_DOUBLE) {
			return CASTABLE_DOUBLE;
		}

		return EXPRSIZE_FLOATING;
	}

	if (right == EXPRSIZE_FLOATING) {
		if (left == CASTABLE_FLOAT) {
			return CASTABLE_FLOAT;
		}

		if (left == CASTABLE_DOUBLE) {
			return CASTABLE_DOUBLE;
		}

		return EXPRSIZE_FLOATING;
	}


	if (left == EXPRSIZE_INTEGER) {
		return signedRight;
	}

	if (right == EXPRSIZE_INTEGER) {
		return signedLeft;
	}

	return left <= right ? signedLeft : signedRight;
}

int chooseFinalSign(int signedSize) {
	if (signedSize == EXPRSIZE_INTEGER)
		return -4; // int

	if (signedSize == EXPRSIZE_FLOATING)
		return 5; // float

	return signedSize;
}
