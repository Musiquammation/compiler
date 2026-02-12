@constructor
@separation
function {
	This.isValueKnown = 0;
	This.positive = 2;
	
}

@constructor
@separation
function def {
	This.val = value;
	This.isValueKnown = 1;

	This.positive = 0;
	if (value >= 0) {
		This.positive = 1;
	}
	
}


@separation
function copy {
	This.val = I.val;
	This.isValueKnown = I.isValueKnown;
	This.positive = I.positive;
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
	DEBUG_PRINT_LINE
}