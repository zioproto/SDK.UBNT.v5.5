var Firewall = new function() {
	this.createRule = function(cfg, objtype, rule) {
		var fwObj = cfg.objPrototypes.createNewObj(objtype, {
			_chain: "FIREWALL",
			_proto: rule.proto,
			_devname : rule.devname,
			_ipProto: rule.ipProto,
			_src: rule.src,
			_sport: (rule.ipProto != 0 ? rule.sport : ""),
			_dst: rule.dst,
			_dport: (rule.ipProto != 0 ? rule.dport : ""),
			_target: rule.target,
			_src_inv: (rule.src.length > 0 ? rule.src_inv : false),
			_sport_inv: (rule.ipProto != 0 && rule.sport ? rule.sport_inv : false),
			_dst_inv: (rule.dst.length > 0 ? rule.dst_inv : false),
			_dport_inv: (rule.ipProto != 0 && rule.dport ? rule.dport_inv : false),
			comment: rule.comment,
			status: rule.enabled ? "enabled" : "disabled" });
		fwObj.cmd = fwObj.buildCmd();
		return fwObj;
	};

	this.addRule = function(objs, rule) {
		objs.push(rule);
		return rule;
	};

	this.removeRule = function(objs, rule) {
		var idx = $.inArray(rule, objs);
		if (idx != -1) {
			objs.splice(idx, 1);
		}
	};

	this.replaceRule = function(objs, oldRule, newRule) {
		var idx = typeof oldRule === "number" ? oldRule : $.inArray(oldRule, objs);
		if (idx != -1) {
			objs[idx] = newRule;
		}
	};

	this.serializeRule = function(cfg, key, rule) {
		cfg.push(key + "status=" + rule.status);
		cfg.push(key + "cmd=" + rule.cmd);
		cfg.push(key + "comment=" + rule.comment);
	};

	this.validateRule = function(rule, errors) {
		var err = errors || [];

		if (!this.validateIp(rule._src))
			err.push('Source IP/Mask is invalid.');
		if (!this.validateIp(rule._dst))
			err.push('Destination IP/Mask is invalid.');
		if (!this.validatePort(rule._sport))
			err.push('Source port is invalid.');
		if (!this.validatePort(rule._dport))
			err.push('Destination port is invalid.');

		return (err.length == 0);
	};

	this.validateIp = function(value) {
		value = value.replace(/^\s+|\s+$/g, '');
		if (!value.length || value == '0.0.0.0/0')
			return true;

		if ((pos = value.indexOf('/')) >= 0) {
			var mask = value.substr(pos+1);
			if (isNaN(mask) || parseInt(mask) != mask || parseInt(mask) < 0 || parseInt(mask) > 32) {
				return false;
			}
			value = value.substr(0, pos);
		}

		return _validateIP(value);
	};

	this.validatePort = function(value) {
		value = value.replace(/^\s+|\s+$/g, '');
		if (!value.length)
			return true;

		if ((pos = value.indexOf(':')) >= 0) {
			var from = value.substr(0, pos);
			var to = value.substr(pos + 1);
			if (isNaN(from) || isNaN(to))
				return false;
			var intfrom = parseInt(from);
			var intto = parseInt(to);
			if (intfrom != from || intto != to || intfrom < 1 ||
					intfrom > 65535 || intto < 1 || intto > 65535 ||
					intfrom > intto)
				return false;
		}
		else if (isNaN(value) || parseInt(value) != value ||
				parseInt(value) < 1 || parseInt(value) > 65535) {
			return false;
		}

		return true;
	};

	this.protoStr = function(proto) {
		switch (proto) {
			case '1': return 'ICMP';
			case '6': return 'TCP';
			case '17': return 'UDP';
			case 'p2p': return 'P2P';
			default: return 'IP';
		}
	};

	this.initRule = function(rule) {
		if (!rule || !rule.cmd || rule.cmd.replace(/^\s+|\s+$/g, '').length == 0)
			return false;
		var params = $.grep(rule.cmd.split(' '), function(p) { return p.length });
		for (var i = 0; i < params.length - 1; ++i) {
			switch (params[i]) {
				case '-t':
					rule._table = params[++i];
					break;
				case '-A':
					rule._chain = params[++i];
					break;
				case '-p':
					rule._proto = params[++i];
					break;
				case '-i':
					rule._devname = params[++i];
					break;
				case '-m':
                                        if (params[++i] == 'ipp2p')
	                                        rule._ipProto = 'p2p';
					break;
				case '--protocol':
				case '--ip-protocol':
					rule._ipProto = params[++i];
					break;
				case '--src':
				case '--ip-src':
					if (params[++i] == '!' && (i + 1) < params.length && i++)
						rule._src_inv = true;
					rule._src = params[i];
					break;
				case '--sport':
				case '--ip-sport':
					if (params[++i] == '!' && (i + 1) < params.length && i++)
						rule._sport_inv = true;
					rule._sport = params[i];
					break;
				case '--dst':
				case '--ip-dst':
					if (params[++i] == '!' && (i + 1) < params.length && i++)
						rule._dst_inv = true;
					rule._dst = params[i];
					break;
				case '--dport':
				case '--ip-dport':
					if (params[++i] == '!' && (i + 1) < params.length && i++)
						rule._dport_inv = true;
					rule._dport = params[i];
					break;
				case '--vlan-id':
					rule._vlan = params[++i];
					break;
				case '-j':
					rule._target = params[++i];
					break;
			}
		}

		return true;
	};

	this.buildIpCmd = function(rule) {
		var cmd = [];

		if (rule._table && rule._table.length)
			cmd.push("-t " + rule._table);

		cmd.push("-A " + rule._chain);

		if (rule._devname.length > 0)
			cmd.push("-i " + rule._devname);

		if (rule._ipProto != 0) {
                	if (rule._ipProto == 'p2p')
                        	cmd.push("-m ipp2p --ipp2p");
                        else
				cmd.push("--protocol " + rule._ipProto);
		}

		if (rule._src.length > 0)
			cmd.push("--src " + (!rule._src_inv ? "" : "! ") + rule._src);

		if (rule._ipProto != 0 && rule._sport.length > 0)
			cmd.push("--sport " + (!rule._sport_inv ? "" : "! ") + rule._sport);

		if (rule._dst.length > 0)
			cmd.push("--dst " + (!rule._dst_inv ? "" : "! ") + rule._dst);

		if (rule._ipProto != 0 && rule._dport.length > 0)
			cmd.push("--dport " + (!rule._dport_inv ? "" : "! ") + rule._dport);

		cmd.push("-j " + (rule._target.length > 0 ? rule._target : "DROP"));
		return cmd.join(" ");
	};


	this.buildEbCmd = function(rule) {
		var cmd = [];

		if (rule._table && rule._table.length)
			cmd.push("-t " + rule._table);

		cmd.push("-A " + rule._chain);

		if (rule._devname.length > 0)
			cmd.push("-i " + rule._devname);

		cmd.push("-p " + rule._proto);
		switch (rule._proto) {
			case "0x0800":
				if (rule._ipProto != 0)
					cmd.push("--ip-protocol " + rule._ipProto);

				if (rule._src.length > 0)
					cmd.push("--ip-src " + (!rule._src_inv ? "" : "! ") + rule._src);

				if (rule._ipProto != 0 && rule._sport.length > 0)
					cmd.push("--ip-sport " + (!rule._sport_inv ? "" : "! ") + rule._sport);

				if (rule._dst.length > 0)
					cmd.push("--ip-dst " + (!rule._dst_inv ? "" : "! ") + rule._dst);

				if (rule._ipProto != 0 && rule._dport.length > 0)
					cmd.push("--ip-dport " + (!rule._dport_inv ? "" : "! ") + rule._dport);
				break;
			case "0x8100":
				if (rule._vlan.length)
					cmd.push("--vlan-id " + rule._vlan);
				break;
		}

		cmd.push("-j " + (rule._target.length > 0 ? rule._target : "DROP"));
		return cmd.join(" ");
	};
};
