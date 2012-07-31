/* Copyright (c) 2010 Ubiquiti Networks, Inc. (http://www.ubnt.com)
 * Collection of helper utilities
 */
$.fn.disable = function(t) {
	t = (t || t == null || t == undefined);
	return this.each(function(){
	        $(this).prop('disabled', t);
	});
};
$.fn.enable = function(enable) {
	return this.disable(enable != null && enable != undefined && !enable);
};

$.fn.toggleDisabled = function() {
	return this.each(function(){
		var d = $(this).prop('disabled');
		$(this).disable(!d);
	});
};

$.fn.fixOverflow = function() {
	if ($.browser.msie) {
		return this.each(function () {
			if (this.scrollWidth > this.offsetWidth) {
					$(this).css({ 'padding-bottom' : '20px', 'overflow-y' : 'hidden' });
				}
			});
		} else {
			return this;
		}
};

$.fn.visible = function(visible) {
	visible = (visible || visible == null || visible == undefined);
	return this.each(function(){
		$(this).css('visibility', visible ? 'visible' : 'hidden');
	});
};
