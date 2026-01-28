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
	this.isValueKnown = 0;
	this.positive = 2;
}