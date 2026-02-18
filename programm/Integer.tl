@control
@argumentConstructor
function {
	this.positive = 2;
	this.isValueKnown = 0;
}

@constructor
@separation
function {
	let mark = 404;
	DEBUG_PRINT_INT mark


	This.isValueKnown = 0;
	This.positive = 2;
}

@constructor
@separation
function def {
	value = value;
	This.val = value;
	This.isValueKnown = 1;

	This.positive = 0;
	if (value >= 0) {
		This.positive = 1;
	}
	
}

@control
function passRuntimePositive {
	let before = this.positive;

	this.positive = 1;

	let mark = 402;
	DEBUG_PRINT_INT mark
	DEBUG_PRINT_CHAR before
}


@control
function cndPositive {
	let mark = 401;
	let sign = this.positive;
	let addr = this.#address;
	
	DEBUG_PRINT_INT mark
	DEBUG_PRINT_CHAR sign
	DEBUG_PRINT_LONG addr

	if (sign == 1) {
		return 1;
	}

	return 0;
}


@separation
function copy {
	This.val = I.val;
	This.isValueKnown = I.isValueKnown;
	This.positive = I.positive;

}

@separation
function give {
	let before = This.positive;
	let mark = 405;

	This.isValueKnown = 0;
	This.positive = 2;

	DEBUG_PRINT_INT mark
	DEBUG_PRINT_CHAR before
}

@separation
function printMeta {
	let mark = 400;

	let val = This.val;
	let isValueKnown = This.isValueKnown;
	let positive = This.positive;

	DEBUG_PRINT_INT mark
	DEBUG_PRINT_INT val
	DEBUG_PRINT_CHAR isValueKnown
	DEBUG_PRINT_CHAR positive
}

@separation
function define {
	let mark = 403;

	This.val = value;
	This.isValueKnown = 1;
	This.positive = 0;
	if (value >= 0) {
		This.positive = 1;
	}

	DEBUG_PRINT_INT mark
	DEBUG_PRINT_INT value
}

@separation
function getSqrt {
	let positive = This.positive;
}

@separation
function getGMean {
	let positive = This.positive;
}

@separation
function printToZero {
	let mark = 405;
	DEBUG_PRINT_INT mark


	while (This.val >= 0) {
		let v = This.val;
		DEBUG_PRINT_INT v

		This.val = This.val - 1;
	}
}

@separation
function setToAbsolute {
	if (This.isValueKnown && This.positive == 0) {
		This.val = 0 -This.val;
	}

	This.positive = 1;
}

