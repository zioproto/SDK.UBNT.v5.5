(function() {

	var ubnt = this.ubnt || (this.ubnt = {}),
	    cfg = ubnt.cfg || (ubnt.cfg = {});

	cfg.version = "1,0,3";
	
	cfg.objPrototypes = {
		base: {
			enabled: function() {
				if (typeof this !== "object")
					return true;
				if (arguments[0] !== undefined)
					this.status = arguments[0] ? "enabled" : "disabled";
				return this.valid() && this.status == "enabled";
			},
			valid: function() {
				return true;
			},
			getDevname: function() {
				if (this.devname !== null)
					return this.devname;
				else
					return "";
			},
			getInterfaces: function() {
				return [];
			},
			find: function(props) {
				if (this.objs === undefined)
					return [];
				return this.grep(function(obj) {
					var res = true;
					$.each(props, function(name, val) {
						if (!(name in obj) || obj[name] !== val) {
							res = false;
							return false;
						}
					});
					return res;
				});
			},
			findByDevname: function(devname, enabled) {
				var objs = this.find({ devname: devname }),
				    res = undefined;
				if (typeof enabled === "boolean") {
					for (var i = 0, length = objs.length; i < length; i++) {
						var o = objs[i];
						if (o && o.enabled() == enabled) {
							res = o;
							break;
						}	
					}
				}
				else if (objs.length)
					res = objs[0];
				return res;
			},
			grep: function(callback) {
				var ret = [];
				if (this.objs) {
					for (var i = 0, length = this.objs.length; i < length; i++) {
						var o = this.objs[i];
						if (o && !!callback(o))
							ret.push(o);
					}
				}
				return ret;
			},
			each: function() {
				if (this.objs) {
					if (typeof arguments[0] === "boolean") {
						var enabled = arguments[0],
						    callback = arguments[1];
						for (var i = 0, length = this.objs.length; i < length; i++) {
							var o = this.objs[i];
							if (o && o.enabled()) {
								if (callback(i, o) === false)
									break;
							}
						}
					}
					else {
						var callback = arguments[0];
						for (var i = 0, length = this.objs.length; i < length; i++) {
							var o = this.objs[i];
							if (o) {
								if (callback(i, o) === false)
									break;
							}
						}
					}
				}
			},
			add: function(obj) {
				if (this.objs) {
					var i = this.objs.length;
					obj.getIdx = function() { return i; }
					this.objs.push(obj);
					return obj;
				}
			},
			remove: function(obj) {
				if (this.objs) {
				var idx = $.inArray(obj, this.objs);
				if (idx != -1)
					this.objs.splice(idx, 1);
				}
			},
			removeAll: function() {
				if (this.objs)
					this.objs = [];
			},
			replace: function(oldObj, newObj) {
				if (this.objs) {
					var idx = typeof oldObj === "number" ? oldObj : $.inArray(oldObj, this.objs);
					if (idx != -1) {
						this.objs[idx] = newObj;
					}
				}
			}
		},
		root: {
			getInterfaces: function(sections) {
				var ifc = [];
				var objs = this;
				$.each(sections, function(idx, section) {
					if (section === 'board') {
						for (var i = 0; i < g_board.board.phycount; i++)
							ifc.push('eth' + i);
						ifc.push('ath0');
					}
					else if (section in objs) {
						var obj = objs[section];
						if (typeof obj === "object" &&
							'getInterfaces' in obj)
							ifc = ifc.concat(obj.getInterfaces());
					}
				});
				// TODO: remove duplicate ifcs
				return ifc;
			},
			get: function(key, createIfNotExist) {
				var create_child = function(r, prefix, section) {
					var c, dot_pos = section.indexOf('.'),
					    k = dot_pos != -1 ? section.substr(0, dot_pos) : section;
					if (/^[0-9]+$/.test(k)) {
						var i = parseInt(k) - 1;
						if (! ("objs" in r))
							r.objs = [];
						if (r.objs[i] === undefined || 
								r.objs[i] == null ||
								typeof r.objs[i] !== "object")
							r.objs[i] = cfg.objPrototypes.createNewObj(prefix+"Obj");
						c = r.objs[i];
						c.getIdx = function() { return i; }
					}
					else {
						if (r[k] === undefined ||
								r[k] == null ||
								typeof r[k] !== "object")
							r[k] = cfg.objPrototypes.createNewObj(k);
						c = r[k];
					}
					return dot_pos != -1 ? create_child(c, k, section.substr(dot_pos+1)) : c;
				    },
				    lookup = function(r, section) {
					var c, dot_pos = section.indexOf('.'),
					    k = dot_pos != -1 ? section.substr(0, dot_pos) : section;
					if (/^[0-9]+$/.test(k)) {
						var i = parseInt(k) - 1;
						if (r.objs && r.objs[i] && typeof r.objs[i] === "object")
							c = r.objs[i];
					}
					else {
						if (r[k] && r[k] && typeof r[k] === "object")
							c = r[k];
					}
					if (c)
						return dot_pos != -1 ? lookup(c, section.substr(dot_pos+1)) : c;
					else
						return undefined;
				    };
				return createIfNotExist === false ? lookup(this, key) : create_child(this, "", key);
			},
			set: function(key, value) {
				// TODO: validate key
				var lastDot = key.lastIndexOf('.'),
				    obj = lastDot != -1 ? this.get(key.substr(0, lastDot)) : this,
				    propName = lastDot != -1 ? key.substr(lastDot+1) : key;
				// Do not replace object with the value.
				// In older version configuration were incorrect keys, ie
				//	wireless.1.wds=enabled
				//	wireless.1.wds.1.peer=
				// In this case wds=enabled will replace whole object.
				if (typeof obj[propName] !== "object")
					obj[propName] = value;
				else
					obj[propName]["val"] = value;
			}
		},
		netconf: {
			status: "disabled",
			create: function(devname, role) {
				var newNetconf = cfg.objPrototypes.createNewObj('netconfObj', {
					devname: devname,
					role: role
				});
				return newNetconf;
			},
			getInterfaces: function() {
				var ifc = [];
				$.each(this.objs, function(idx, obj) {
					if (obj !== undefined && obj.devname.length)
						ifc.push(obj.devname);
				});
				return ifc;
			}
		},
		netconfObj: {
			status: "enabled",
			devname: "",
			mtu: "1500",
			role: "",
			autoip: {
				status: "disabled"
			},
			alias: {
				objs: [],

				create: function(ip, netmask, comment, enabled) {
					var aliasObj = cfg.objPrototypes.createNewObj('aliasObj', {
						ip: ip.replace(/^\s+|\s+$/g, ''),
						netmask: netmask.replace(/^\s+|\s+$/g, ''),
						comment: comment,
						status: enabled ? "enabled" : "disabled" });
					return aliasObj;
				},
				add: function(aliasObj) {
					this.objs.push(aliasObj);
					return aliasObj;
				},
				remove: function(aliasObj) {
					var idx = $.inArray(aliasObj, this.objs);
					if (idx != -1) {
						this.objs.splice(idx, 1);
					}
				},
				init: function() {
					this.objs = this.grep(function(obj) { return obj.valid(); });
				}
			},
			hwaddr: {
				status: "disabled",
				mac: ""
			},
			clear: function() {
				this.ip = "0.0.0.0";
				this.netmask = "255.255.255.0";
				this.alias.removeAll();
				this.autoip.enabled(false);
				this.hwaddr.enabled(false);
			}
		},
		aliasObj: {
			status: "disabled",
			ip: "",
			netmask: "",
			comment: "",
			valid: function(errors) {
				var err = errors || [];

				if (!this.ip.length)
					err.push('IP Address can not be empty.');
				else if (!_validateNonZeroIP(this.ip))
					err.push('IP Address is invalid.');

				if (!this.netmask.length)
					err.push('Netmask can not be empty.');
				else if (!_validateNetmask(this.netmask))
					err.push('Netmask is invalid.');

				return (err.length == 0);
			}
		},
		bridge: {
			status: "disabled",
			objs: [],
			add: function() {
				var newNo = 0;
				$.each(this.objs, function(idx, bridgeObj) {
					var no = parseInt(bridgeObj.devname.substr(2));
					if (no >= newNo)
						newNo = no + 1;
				});
				var newbridge = cfg.objPrototypes.createNewObj('bridgeObj', {
					devname: "br" + newNo });
				this.objs.push(newbridge);
				return newbridge;
			},
			remove: function(bridgeObj) {
				var idx = $.inArray(bridgeObj, this.objs);
				if (idx != -1)
					this.objs.splice(idx, 1);
			},
			getInterfaces: function() {
				var ifc = [];
				$.each(this.objs, function(idx, obj) {
					if (obj !== undefined && obj.devname.length)
						ifc.push(obj.devname);
				});
				return ifc;
			},
			getPorts: function() {
				var ifc = [];
				$.each(this.objs, function(idx, obj) {
					$.merge(ifc, obj.getPorts());
				});
				return ifc;
			}
		},
		bridgeObj: {
			status: "enabled",
			devname: "",
			stp: {
				status: "disabled"
			},
			port: {
				objs: []
			},
			findPort: function(devname) {
				var portObj = this.port.find({ devname: devname }),
				    portIdx = -1;
				if (portObj.length) {
					portIdx = $.inArray(portObj[0], this.port.objs);
				}
				return portIdx;
			},
			addPort: function(devname) {
				var portIdx = this.findPort(devname);
				if (portIdx == -1) {
					var newPort = cfg.objPrototypes.createNewObj('portObj', {
						devname: devname, 
						status: "enabled" });
					this.port.objs.push(newPort);
					return newPort;
				}
				else if (!this.port.objs[portIdx].enabled()) {
					var port = this.port.objs[portIdx];
					port.enabled(true);
					return port;
				}
				return null;
			},
			removePort: function(devname) {
				var idx = this.findPort(devname);
				if (idx != -1)
					this.port.objs.splice(idx, 1);
			},
			getPorts: function() {
				var ifc = [];
				$.each(this.port.objs, function(idx, portObj) {
					if (portObj.valid() && portObj.enabled())
						ifc.push(portObj.getDevname());
				});
				return ifc;
			},
			canHavePort: function(devname) {
				// vlan and parent interface can't be in the same bridge
				var can = true;
				if (cfg.objPrototypes.vlan.isVlan(devname)) {
					can = ($.inArray(cfg.objPrototypes.vlan.getParent(devname), this.getPorts()) == -1) &&
					      (devname.indexOf('br') != 0);
				}
				else {
					$.each(this.getPorts(), function(idx, p) {
						if (p.indexOf(devname) == 0) {
							can = false;
							return false;
						}
					});
				}
				return can;
			}
		},
		portObj: {
			status: "enabled",
			devname: "",
			valid: function() {
				return this.devname.length > 0;
			}
		},
		vlan: {
			status: "disabled",
			objs: [],
			create: function(devname, id, comment, enabled) {
				var newVlan = cfg.objPrototypes.createNewObj('vlanObj', {
							devname: devname, 
							id: id, 
							comment: comment, 
							status: enabled ? "enabled" : "disabled" });
				return newVlan;
			},
			isVlan: function(ifc) {
				return ifc.indexOf('.') != -1;
			},
			getParent: function(ifc) {
				return ifc.split('.')[0];
			},
			getId: function(ifc) {
				return ifc.split('.')[1];
			},
			findByFullDevname: function(vlanDevname) {
				var vlans = this.find({ devname: this.getParent(vlanDevname),
							id: this.getId(vlanDevname) });
				return vlans.length ? vlans[0] : undefined;
			},
			getInterfaces: function() {
				var ifc = [];
				if (this.objs) {
				$.each(this.objs, function(idx, obj) {
					ifc.push(obj.getFullDevname());
				});
				}
				return ifc;
			},
			init: function() {
				this.objs = this.grep(function(obj) { return obj.valid(); });
			}
		},
		vlanObj: {
			status: "enabled",
			devname: "",
			id: "",
			comment: "",
			getFullDevname: function() {
				return this.devname.length && this.id.length ?
					this.devname + "." + this.id : "";
			},
			valid: function(errors) {
				var err = errors || [],
				    id = parseInt(this.id);
				if (!this.id.length || !/^\d+$/.test(this.id) ||
				    id < 2 || id > 4094)
					err.push('Please enter a valid "VLAN ID [2 - 4094]".');
				return (err.length == 0);
			}
		},
		route: {
			status: "disabled",
			create: function(ip, netmask, gateway, comment, enabled) {
				var routeObj = cfg.objPrototypes.createNewObj('routeObj', {
					ip: ip.replace(/^\s+|\s+$/g, ''),
					netmask: netmask.replace(/^\s+|\s+$/g, ''),
					gateway: gateway.replace(/^\s+|\s+$/g, ''),
					comment: comment,
					status: enabled ? "enabled" : "disabled" });
				return routeObj;
			},
			add: function(routeObj) {
				this.objs.push(routeObj);
				return routeObj;
			},
			remove: function(routeObj) {
				var idx = $.inArray(routeObj, this.objs);
				if (idx != -1) {
					this.objs.splice(idx, 1);
				}
			},
			init: function() {
				this.objs = this.grep(function(obj) { return obj.ip.length > 0; });
			}
		},
		routeObj: {
			ip: "",
			netmask: "",
			gateway: "",
			comment: "",
			status: "enabled",
			valid: function(errors) {
				var err = errors || [];

				if (!this.ip.length)
					err.push('Target Network IP can not be empty.');
				else if (!_validateNonZeroIP(this.ip))
					err.push('Target Network IP is invalid.');

				if (!this.netmask.length)
					err.push('Netmask can not be empty.');
				else if (!_validateNetmask(this.netmask))
					err.push('Netmask is invalid.');

				if (!this.gateway.length)
					err.push('Gateway IP can not be empty.');
				else if (!_validateNonZeroIP(this.gateway))
					err.push('Gateway IP is invalid.');

				if (err.length > 0) /* makes no sense to check more */
					return false;

				if (!_validateRouteTarget(this.ip, this.netmask))
					err.push('Target Network IP / Netmask is invalid.');

				return (err.length == 0);
			}
		},
		resolv: {
			status: "disabled"
		},
		nameserver: {
			create: function(ip, enabled) {
				var nsObj = cfg.objPrototypes.createNewObj('nameserverObj', {
					ip: ip.replace(/^\s+|\s+$/g, ''),
					status: enabled ? "enabled" : "disabled" });
				return nsObj;
			}
		},
		nameserverObj: {
			ip: "",
			status: "enabled"
		},
		iptables : {
			status : "disabled",
			objs: [],
			init: function() {
				this.objs = this.grep(Firewall.initRule);
			},
			create: function(rule) {
				return Firewall.createRule(cfg, "iptablesObj", rule);
			},
			add: function(fwObj) {
				return Firewall.addRule(this.objs, fwObj);
			},
			remove: function(fwObj) {
				return Firewall.removeRule(this.objs, fwObj);
			},
			replace: function(oldObj, newObj) {
				return Firewall.replaceRule(this.objs, oldObj, newObj);
			}
		},
		iptablesObj : {
			status : "disabled",
			cmd : "",
			comment : "",

			_table: "",
			_chain: "",
			_proto: "",
			_devname : "",
			_ipProto: 0,
			_src: "",
			_sport: "",
			_dst: "",
			_dport: "",
			_target: "",

			/* inversion flags */
			_src_inv: false,
			_sport_inv: false,
			_dst_inv: false,
			_dport_inv: false,

			valid: function(errors) {
				return Firewall.validateRule(this, errors);
			},
			_ipProtoStr: function() {
				return Firewall.protoStr(this.data._ipProto);
			},
			serialize: function(cfg, key) {
				return Firewall.serializeRule(cfg, key, this);
			},
			buildCmd: function() {
				return Firewall.buildIpCmd(this);
			}
		},
		ebtables : {
			status : "disabled",
			objs: [],
			init: function() {
				this.objs = this.grep(Firewall.initRule);
			},
			create: function(rule) {
				return Firewall.createRule(cfg, "ebtablesObj", rule);
			},
			add: function(fwObj) {
				return Firewall.addRule(this.objs, fwObj);
			},
			remove: function(fwObj) {
				return Firewall.removeRule(this.objs, fwObj);
			},
			replace: function(oldObj, newObj) {
				return Firewall.replaceRule(this.objs, oldObj, newObj);
			}
		},
		ebtablesObj : {
			status : "disabled",
			cmd : "",
			comment : "",

			_table: "",
			_chain: "",
			_proto: "",
			_devname : "",
			_ipProto: 0,
			_src: "",
			_sport: "",
			_dst: "",
			_dport: "",
			_target: "",

			/* inversion flags */
			_src_inv: false,
			_sport_inv: false,
			_dst_inv: false,
			_dport_inv: false,

			valid: function(errors) {
				return Firewall.validateRule(this, errors);
			},

			_ipProtoStr: function() {
				return Firewall.protoStr(this.data._ipProto);
			},
			serialize: function(cfg, key) {
				return Firewall.serializeRule(cfg, key, this);
			},
			buildCmd: function() {
				return Firewall.buildEbCmd(this);
			}
		},
		dhcpc: {
			status: "disabled",
			objs: [],
			create: function(devname, ip, netmask, enabled) {
				var dhcpcObj = cfg.objPrototypes.createNewObj('dhcpcObj', {
					devname: devname,
					fallback: ip.replace(/^\s+|\s+$/g, ''),
					fallback_netmask: netmask.replace(/^\s+|\s+$/g, ''),
					status: enabled ? "enabled" : "disabled" });
				return dhcpcObj;
			}
		},
		ppp: {
			status: "disabled",
			objs: [],
			create: function(devname, user, psw, service, mppe, mtu, mru, enabled) {
				var pppObj = cfg.objPrototypes.createNewObj('pppObj', {
					devname: devname,
					name: user,
					password: psw,
					pppoe_service: service,
					require: {
						mppe128: mppe ? "enabled" : "disabled"
					},
					mtu: mtu,
					mru: mru
				});
				pppObj.enabled(enabled);
				return pppObj;
			}
		},
		masq: {
			status: "disabled",
			objs: [],
			create: function(devname, enabled) {
				var masqObj = cfg.objPrototypes.createNewObj('masqObj', {
					devname: devname
				});
				masqObj.enabled(enabled);
				return masqObj;
			}
		},
		dhcpd: {
			status: "disabled",
			objs: [],
			create: function(devname, enabled) {
				var dhcpdObj = cfg.objPrototypes.createNewObj('dhcpdObj', {
					devname: devname
				});
				dhcpdObj.enabled(enabled);
				return dhcpdObj;
			}
		},
		dhcpdObj: {
			status: "enabled",
			dns: {
				objs: [
					{ server: "" },
					{ server: "" }
				]
			}
		},
		client: {
			objs: [],
			create: function(devname, enabled) {
				var clientObj = cfg.objPrototypes.createNewObj('clientObj', {
					devname: devname
				});
				clientObj.enabled(enabled);
				return clientObj;
			}
		},
		server: {
			objs: [],
			create: function(devname, enabled) {
				var serverObj = cfg.objPrototypes.createNewObj('serverObj', {
					devname: devname
				});
				serverObj.enabled(enabled);
				return serverObj;
			}
		},
		blacklist: {
			objs: [],
			create: function(expr, enabled) {
				var blObj = cfg.objPrototypes.createNewObj('blacklistObj', {
					expr: expr
				});
				blObj.enabled(enabled);
				return blObj;
			},
			init: function() {
				this.objs = this.grep(function(obj) { return obj.valid(); });
			}
		},
		blacklistObj: {
			valid: function() {
				return this.expr;
			}
		},
		dmz: {
			objs: [],
			create: function(devname, enabled) {
				var obj = cfg.objPrototypes.createNewObj('dmzObj', {
					devname: devname
				});
				obj.enabled(enabled);
				return obj;
			}
		},
		dmzObj: {
			except: {}
		},
		except: {
			objs: [],
			create: function(port, proto, enabled) {
				var obj = cfg.objPrototypes.createNewObj('exceptObj', {
					port: port,
					proto: proto
				});
				obj.enabled(enabled);
				return obj;
			}
		},
		mgmt: {
			objs: [],
			create: function(devname, enabled) {
				var obj = cfg.objPrototypes.createNewObj('mgmtObj', {
					devname: devname
				});
				obj.enabled(enabled);
				return obj;
			}
		},
		dmzObj: {
			except: {}
		},
		portfw: {
			objs: [],
			create: function(devname, host, port, proto, src, dst, dport, comment, enabled) {
				var obj = cfg.objPrototypes.createNewObj('portfwObj', {
					devname: devname,
					host: host,
					port: port,
					proto: proto,
					src: src || "0.0.0.0/0",
					dst: dst || "0.0.0.0/0",
					dport: dport,
					comment: comment
				});
				obj.enabled(enabled);
				return obj;
			}
		},
		portfwObj: {
			valid: function(errors) {
				var err = errors || [],
				    dports = this.dport.split(':', 2);

				if (!this.host || !Firewall.validateIp(this.host))
					err.push('Private IP is invalid.');
				if (this.port && !Firewall.validatePort(this.port))
					err.push('Private port is invalid.');
				if (!Firewall.validateIp(this.src))
					err.push('Source IP/Mask is invalid.');
				if (!Firewall.validateIp(this.dst))
					err.push('Public IP/Mask is invalid.');
				if (!this.dport ||
				    (dports.length == 1 && 
				       !Firewall.validatePort(this.dport)) ||
				    (dports.length == 2 && 
				      (!dports[1] ||
				       !Firewall.validatePort(dports[0]) ||
				       !Firewall.validatePort(dports[1])) ||
				       parseInt(dports[0]) > parseInt(dports[1])))
					err.push('Public port is invalid.');

				return (err.length == 0);
			}
		},
		arpnat: {
			objs: [],
			create: function(devname, enabled) {
				var obj = cfg.objPrototypes.createNewObj('arpnatObj', {
					devname: devname
				});
				obj.enabled(enabled);
				return obj;
			}
		},
		eap: {
			objs: [],
			create: function(devname, enabled) {
				var obj = cfg.objPrototypes.createNewObj('eapObj', {
					devname: devname
				});
				obj.enabled(enabled);
				return obj;
			}
		},
		igmpproxy: {
			objs: [],
			upstream: { devname: undefined },
			create: function(devname, enabled) {
				var obj = cfg.objPrototypes.createNewObj('igmpproxyObj', {
					downstream: {
						devname: devname
					}
				});
				return obj;
			}
		},
		tshaper: {
			status: "disabled",
			objs: [],
			create: function(devname, inEnabled, inRate, inBurst, outEnabled, outRate, outBurst, enabled) {
				var obj = cfg.objPrototypes.createNewObj('tshaperObj', {
							devname: devname, 
							input: { rate: inRate || "0", burst: inBurst || "0" }, 
							output: { rate: outRate || "0", burst: outBurst || "0" }
						});
				obj.enabled(enabled);
				obj.input.enabled(inEnabled);
				obj.output.enabled(outEnabled);
				return obj;
			},
			getInterfaces: function() {
				var ifc = [];
				$.each(this.objs, function(idx, obj) {
					if (obj !== undefined && obj.devname.length)
						ifc.push(obj.devname);
				});
				return ifc;
			}
		},
		tshaperObj: {
			status: "enabled",
			input: { status: "enabled" },
			output: { status: "enabled" },
			valid: function(errors) {
				var err = errors || [],
				    inRate = parseInt(this.input.rate),
				    inBurst = parseInt(this.input.burst),
				    outRate = parseInt(this.output.rate),
				    outBurst = parseInt(this.output.burst);
				if (this.input.enabled()) {
					if (!this.input.rate || inRate < 32 || inRate > 90000 || 
                                        	isNaN(inRate) || this.input.rate != inRate)
						err.push('Please enter a valid Ingress Traffic Rate [32-90000].');
					var maxBurst = this.input.rate * 8;
					if (!this.input.burst || inBurst < 0 || inBurst > maxBurst || 
                                        	isNaN(inBurst) || this.input.burst != inBurst)
						err.push('Please enter a valid Ingress Traffic Burst [0-'+parseInt(maxBurst)+'].');
				}
				if (this.output.enabled()) {
					if (!this.output.rate || outRate < 32 || outRate > 90000 || 
                                        	isNaN(outRate) || this.output.rate != outRate)
						err.push('Please enter a valid Egress Traffic Rate [32-90000].');
					var maxBurst = this.output.rate * 4;
					if (!this.output.burst || outBurst < 0 || outBurst > maxBurst || 
                                        	isNaN(outBurst) || this.output.burst != outBurst)
						err.push('Please enter a valid Egress Traffic Burst [0-'+parseInt(maxBurst)+'].');
				}
				if (!this.input.enabled() && !this.output.enabled()) {
						err.push('Please select Traffic Direction.');
				}
				return (err.length == 0);
			},
			init: function() {
				var inRate = parseInt(this.input.rate),
				inBurst = parseInt(this.input.burst),
				outRate = parseInt(this.output.rate),
				outBurst = parseInt(this.output.burst);
				if (this.input.enabled()) {
					if (!this.input.rate || inRate < 32 || isNaN(inRate))
						this.input.rate = 32;
					if (inRate > 90000)
						this.input.rate = 90000;
					if (!this.input.burst || inBurst < 0 || isNaN(inBurst))
						this.input.burst = 0;
					var maxBurst = this.input.rate * 8;
					if (inBurst > maxBurst)
						this.input.burst = maxBurst;
                                }
				if (this.output.enabled()) {
					if (!this.output.rate || outRate < 32 || isNaN(outRate))
						this.output.rate = 32;
					if (outRate > 90000)
						this.output.rate = 90000;
					if (!this.output.burst || outBurst < 0 || isNaN(outBurst))
						this.output.burst = 0;
                                        var maxBurst = this.output.rate * 4;
					if (outBurst > maxBurst)
						this.output.burst = maxBurst;
				}
			}
		},
		mac_acl: {
			status: "disabled",
			objs: [],
			create: function(mac, comment, enabled) {
				var newMacacl = cfg.objPrototypes.createNewObj('mac_aclObj', {
							mac: mac, 
							comment: comment, 
							status: enabled ? "enabled" : "disabled" });
				return newMacacl;
			},
			init: function() {
				this.objs = this.grep(function(obj) { return obj.valid(); });
			}
		},
		mac_aclObj: {
			status: "enabled",
			mac: "",
			comment: "",
			valid: function(errors) {
				var err = errors || [];
				if (!this.mac || 
				    /^([0-9a-fA-F][0-9a-fA-F]:){5}([0-9a-fA-F][0-9a-fA-F])$/.exec(this.mac) == null)
					err.push('Please enter a valid "MAC".');
				return (err.length == 0);
			}
		},
		upnpd: {
			status: "enabled",
			devname: "",
			listening: {
				objs: [],

				create: function(ip, netmask, enabled) {
					var obj = cfg.objPrototypes.createNewObj('listeningObj', {
						ip: ip.replace(/^\s+|\s+$/g, ''),
						netmask: netmask.replace(/^\s+|\s+$/g, ''),
						status: enabled ? "enabled" : "disabled" });
					return obj;
				}
			}
		},
		'3gObj': {
			status: "enabled",
			pin: {
				status: "disabled",
				code: "",
				iccid: ""
			}
		},
		createNewObj: function(objName, initialValues) {
			var newObj = $.extend(true, {}, cfg.objPrototypes.base, cfg.objPrototypes[objName], initialValues);
			// Add base functionality for nested objects
			for (propName in newObj) {
				prop = newObj[propName];
				if (typeof prop === "object") {
					$.extend(prop, cfg.objPrototypes.base);
					if (cfg.objPrototypes[propName])
						$.extend(prop, cfg.objPrototypes[propName]);
				}
			}
			return newObj;
		}
	};

	cfg.mkVersion = function() {
		var p = cfg.version.split(','),
		    num = 0;
		for (var n in p)
			num = (num << 8) | parseInt(p[n]);
		return num;
	}

	cfg.parse = function(config) {
		var cfg_lines = config.split(/\r\n|\r|\n/),
		    json_result = $.extend({}, cfg.objPrototypes.root),
		    callInit = function(r) {
			$.each(r, function(prop, obj) {
				if (typeof obj === "object") {
					callInit(obj);
					if (typeof obj.init === "function")
						obj.init();
				}
				else if (typeof obj === "array") {
					$.each(obj, callInit);
				}
			});
		    };
		for (idx in cfg_lines) {
			var line = cfg_lines[idx],
			    eqPos = line.indexOf('=');
			if (eqPos != -1)
				json_result.set(line.substr(0, eqPos), line.substr(eqPos+1));
		}
		callInit(json_result);
		return json_result;
	};

	cfg.toCfg = function(jsonCfg) {
		var result = [];
		var objToString = function(key, o) {
			var t = typeof o;
			if (t === "number")
				result.push(key + "=" + o.toString());
			else if (t === "string")
				result.push(key + "=" + o);
			else if (t === "object") {
				if (o["val"])
					result.push(key + "=" + o["val"]);

				if (key.length > 0)
					key += ".";
				if (typeof o.serialize == "function") {
					o.serialize(result, key);
				}
				else {
					for (var p in o)
					{
						if (p == "objs")
							$.each(o.objs, function(i, objs) {
								objToString(key + (i + 1), objs);
							});
						else if (p != "val")
							objToString(key + p, o[p]);
					}
				}
			}
		};
		jsonCfg.set('system.cfg.version', cfg.mkVersion());
		objToString("", jsonCfg);
		return result;
	};

	cfg.devname2uidevname = function(devname) {
	    var res = devname.replace("ath","WLAN");
	    res = res.replace("eth","LAN");
	    res = res.replace("br","BRIDGE");
	    res = res.replace("ppp+","PPP");
	    return res;
	};

	cfg.uidevname2devname = function(uidevname) {
	    var res = uidevname.replace("WLAN","ath");
	    res = res.replace("LAN","eth");
	    res = res.replace("BRIDGE","br");
	    res = res.replace("PPP","ppp+");
	    return res;
	};

})();
