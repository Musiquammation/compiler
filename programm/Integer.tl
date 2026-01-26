@separation
function define {
	this.val = value;
	this.isValueKnown = 1;

	this.positive = 0;
	if (value >= 0) {
		this.positive = 1;
	}

	if (value < 0) {
		this.positive = 2;
	}
}