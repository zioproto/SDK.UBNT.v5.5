/* Copyright (c) 2010 Ubiquiti Networks, Inc. (http://www.ubnt.com)
 * Yet another plugin to show/hide password fields.
 * Tested on jQuery 1.3.2
 */
;(function($){
	$.fn.passwd = function(options){
		var opts = $.extend({}, $.fn.passwd.defaults, options);

		$.fn.passwd.toggler = function(chkbox, x1, x2){
			chkbox.click(function(){
				if ($(this).attr('checked')) {
					inputSwap(x1, x2, opts);
					x1.hide();x2.show();
				} else {
					inputSwap(x2, x1, opts);
					x2.hide();x1.show();
				}
			});
		};

		return this.each(function(){
			// name attribute must match the id, to avoid any stupidities
			if ($(this).attr('name') != $(this).attr('id')) {
				$(this).attr('name', $(this).attr('id'));
			}
			var tid = '_' + this.id + '_t';
			// clone() + input type change does not work on IE...
			var id = '_' + this.id + '_x';
			var cloned = inputClone($(this), id, opts.clone_attrs, opts.clone_styles);
			$(this).before(cloned);
			cloned.hide();
			var toggler = $.fn.passwd.create_toggler($(this), tid, opts);
			$.fn.passwd.toggler(toggler, $(this), cloned);
		});
	};

	function inputClone($x, $id, attrs, styles){
		var pwd = $('<input type="text" id="' + $id + '" name="' + $id + '" value="" />');
		jQuery.each(attrs, function(i, val){ pwd.attr(val, $x.attr(val)); });
		jQuery.each(styles, function(i, val){ pwd.css(val, $x.css(val)); });
		pwd.css('display', $x.css('display'));
		pwd.val($x.val());
		return pwd;
	};
	function inputSwap($x1, $x2, opts){
		var id = $x1.attr('id');
		var name = $x1.attr('name');
		$x2.val($x1.val());
		$x1.attr('id', $x2.attr('id')).attr('name', $x2.attr('name'));
		$x2.attr('id', id).attr('name', name);
		jQuery.each(opts.clone_attrs, function(i, name){ $x2.attr(name, $x1.attr(name)); });
		jQuery.each(opts.migrate_attrs, function(i, name){ $x2.attr(name, $x1.attr(name)); $x1.removeAttr(name); });
	};
	function debug($msg) {
		if (window.console && window.console.log)
			window.console.log('DEBUG: ' + $msg);
	};

	$.fn.passwd.create_toggler = function(target, id, opts) {
		var toggler = $('<input type="checkbox" id="' + id + '"/>');
		var label = $('<label for="' + id + '">' + opts.label +'</label>');
		target.after(label).after(toggler);
		return toggler;
	};

	$.fn.passwd.defaults = {
		label: 'Show',
		clone_attrs: [ 'class', 'disabled', 'autocomplete' ],
		migrate_attrs: [],
		clone_styles: []
	};
})(jQuery);
