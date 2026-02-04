@control
@argumentConstructor
function {
	this.positive = 2;
	this.isValueKnown = 0;

	let u = 10012002;
	DEBUG_PRINT_INT u
}


@control
function cndPositive {
	let sign = this.positive;
	DEBUG_PRINT_CHAR sign

	if (sign == 1) {
		return 1;
	}

	return 0;
}

@separation
function getSqrt {
	
}

@separation
function define {
	this.val = value;
	this.isValueKnown = 1;

	this.positive = 0;
	if (value >= 0) {
		this.positive = 1;
	}
}

@constructor
@separation
function {
	this.val = 94700;
	let u = this.val;
	DEBUG_PRINT_INT u
}


@constructor
@separation
function def {
	let tmp = 42001;
	DEBUG_PRINT_INT tmp
	DEBUG_PRINT_INT value

	this.val = value;
	this.isValueKnown = 1;

	this.positive = 0;
	if (value >= 0) {
		this.positive = 1;
	}

}

