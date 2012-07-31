(function() {

	var ubnt = this.ubnt || (this.ubnt = {}),
	    core = ubnt.core || (ubnt.core = {});

	core.events = {
		bind: function(type, fn) {
			var types = type.split(" "),
			    fns = this.evFns || (this.evFns = {});
			for (var i = 0; i < types.length; i++)
			    (this.evFns[types[i]] || (this.evFns[types[i]] = [])).push(fn);
			return this;
		},
		ubnind: function(type, fn) {
			var fns, typeFns;
			if (!(fns = this.evFns) || !(typeFns = fns[type]))
				return this;
			if (fn) {
				for (var i = 0, l = typeFns.length; i < l; i++) {
					if (fn === typeFns[i]) {
						typeFns.splice(i, 1);
						break;
					}
				}
			}
			else {
				delete this.evFns[type];
			}
			return this;
		},
		trigger: function(type) {
			var fns, typeFns;
			if (!(fns = this.evFns) || !(typeFns = fns[type]))
				return this;
			for (var i = 0, l = typeFns.length; i < l; i++)
				typeFns[i].apply(this, Array.prototype.slice.call(arguments, 1));
			return this;
		}
	}
})();
