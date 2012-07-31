/*! Localisation support for jQuery through simple map dictionary.
 * Supports java-like ({n}) and sprintf-like (%s, %d) placeholders.
 *
 * Copyright (c) 2010 Ubiquiti Networks, Inc. (http://www.ubnt.com)
 * Dual licensed under the MIT or GPL Version 2 licenses.
 * Tested on jQuery 1.4.2
 */
;(function($){
	$.extend({ l10n: {
		init: function(options) {
			var opts = $.extend({}, $.l10n.defaults, options);
			this.dict = opts.dictionary;
			this.name = opts.name;
			this.set_formatter(opts.formatter);
		},
		set_formatter: function(f) {
			switch (f) {
			case 'java':
				this.format = format_java;
				this.formatter = f;
				break;
			case 'sprintf':
				this.format = format_sprintf;
				this.formatter = f;
				break;
			case 'none':
			default:
				this.format = format_none;
				this.formatter = 'none';
			}
		},
		get: function(word, args) {
			var key = word;
			var trans = word;
			if (/\|/.test(word)) {
				var a = word.split('|');
				key = a[0]; a.shift();
				trans = a.join('|');
			}
			var x = this.dict[key];
			trans = x || trans;
			return this.format(trans, args);	
		},
		_: function(word, args) {
			return this.get(word, args);
		}
	}});
	/* simple java MessageFormat style formatting, {n} replacement */
	function format_java(str,args) {
		if (!args || args.length == 0 || !RegExp) return str;
		var re = ['\\{', '1', '\\}'];
		for (i = 0; i < args.length; ++i) {
			re[1] = i;
			var r = new RegExp(re.join(''), 'g');
			str = str.replace(r, args[i]);
		}
		return str;
	};
	/* sprintf-like formatting, limited now to %s, %d */
	function format_sprintf(str,args) {
		if (!args || args.length == 0 || !RegExp) return str;
		var regex = /([^%]*)%([sd])(.*)/;
		var i = 0; var re = [];
		while (i < args.length && (re = regex.exec(str))) {
			re.shift(); re[1] = args[i];
			str = re.join('');
			i++;
		}
		return str;
	};
	/* no formatting */
	function format_none(str,args) {
		return str;
	};
	$.l10n.defaults = {
		dictionary: { },
		name: 'default',
		formatter: 'sprintf'
	};
})(jQuery);
