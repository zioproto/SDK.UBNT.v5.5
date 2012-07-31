function Progress(id, dotImage, length, width, pulsing) {
        var c = document.getElementById(id);
        if (!c) return;

	if (length == undefined || isNaN(parseInt(length)) || length == 0) {
		length = 100;
	}
	if (width == undefined || isNaN(parseInt(width))) {
		width = 12;
	}
	this.pulsing = pulsing == undefined ? 0 : pulsing;
	this.current = 0;
	this.length = length;
	this.width = width;

	this.calculateLength = function(val) {
		var percent = parseInt(val);
		if (isNaN(percent)) {
			percent = 0;
		}
		if (percent > 100) {
			percent = 100;
		}
		this.current = percent;
		var w = percent * this.length / 100;
		return w;
	}
	this.calculatePulse = function(val) {
		var p = parseInt(val);
		if (isNaN(p)) p = 0;
		if (p > 100) p = 100;
		this.current = p;
		offset = p * this.length / 100;
		if (offset + this.image.width >= this.length) {
			this.sign *= -1;
			offset = this.length - this.image.width;
		} else if (offset <= 0) {
			this.sign *= -1;
			offset = 0;
		}
		return offset;
	}
	this.set = function(val) {
		if (this.pulsing) {
			this.image.style.left=this.calculatePulse(val) + "px";
		} else {
			var w = this.calculateLength(val);
			this.image.width = w;
			this.image.title = this.current + " %";
			this.image.alt = this.current + " %";
		}
	}
	this.run = function(duration, endCallback) {
		this.pulsing = 0;
		this.set(0);
		this.stop = false;
		this.step = duration / 100;
		this.endCallback = endCallback;
		this.schedule();
	}
	this.pulse = function(speed, movestep, width) {
		this.pulsing = movestep == undefined ? 2 : movestep;
		this.stop = false;
		this.step = speed;
		this.sign = 1;
		this.image.width = width == undefined ? this.length / 5 : width;
		this.image.style.position="relative";
		this.schedule();
	}
	this.schedule = function() {
		var oThis = this;
		if (this.stop) return;
		this.t = window.setTimeout(function() {
			if (oThis.pulsing) {
				oThis.set(oThis.current + oThis.pulsing * oThis.sign);
				oThis.schedule();
			} else {
				if (oThis.current < 100) {
					oThis.set(oThis.current + 1);
					oThis.schedule();
				} else {
					if (oThis.endCallback != undefined)
						eval(oThis.endCallback);
				}
			}
		}, this.step);
	}
	this.cancel = function() {
		this.stop = true;
		if (this.t)
			window.clearTimeout(this.t);
	}

	this.image = document.createElement("img");
	this.image.src = dotImage;
	this.image.width = 0;
	this.image.height = this.width;

	c.style.width = length + "px";
	c.appendChild(this.image);
}
