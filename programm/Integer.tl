@control
@argumentConstructor
function {
	let u = 10012002;
	DEBUG_PRINT_INT u
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
	this.val = value;
	this.isValueKnown = 1;

	this.positive = 0;
	if (value >= 0) {
		this.positive = 1;
	}
}

