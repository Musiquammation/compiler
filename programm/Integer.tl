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
	let u: int;
	u = this.val;	
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