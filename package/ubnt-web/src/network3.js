
var g_cfg = {},
    views = {

netmode: {
	headerId: '#headerNetmode',
	init: function() {
		var that = this;
		this.cfgBackup = {};
		$('#netmode').change(function() {
			var newmode = $(this).val(),
			    oldCfg = g_cfg,
			    netEvFns = g_net.evFns;
			$('#network').trigger('beforeSubmit');
			if (that.cfgBackup[newmode]) {
				g_cfg = that.cfgBackup[newmode];
			}
			else {
				g_cfg = g_net.switchNetmode(newmode);
			}
			that.cfgBackup[oldCfg.netmode] = oldCfg;
			g_net = ubnt.net.network(g_cfg);
			// XXX: when changing network mode, new network object is created
			// and it will have no event bindings. Quick solution is to use
			// previous bindings.
			g_net.evFns = netEvFns;
			renderViews();
		});
	},
	render: function() {
	}
},
disableNet: {
	init: function() {
		$('#network')
			.bind('beforeSubmit', function() {
				var val = $('#disableNet').val(),
				    wanDevname = g_net.isBridge ? "" : g_net.wan.getDevname(),
				    disabled = [];
				if (val == 'eth0/eth1')
					disabled = ['eth0', 'eth1'];
				else if (val == 'eth0/ath0')
					disabled = ['eth0', 'ath0'];
				else
					disabled.push(val);
				g_cfg.netconf.each(function(idx, netconfObj) {
					var enabled = $.inArray(netconfObj.devname, disabled) == -1;
					if (enabled && g_net.isBridge && netconfObj.up == "disabled") {
						// XXX: assuming that there is only one bridge
						// In v5.3 disabling interface, disables bridge port.
						// Reenable it.
						var bridge = g_cfg.bridge.objs[0],
						    port = bridge && bridge.port.findByDevname(netconfObj.devname);
						if (port && bridge.canHavePort(port.devname))
							port.enabled(true);
					}
					netconfObj.up = enabled ? "enabled" : "disabled";
					// TODO: Disable bridge port for disabled interface 
				});
			});
	},
	render: function() {
		var vals = ['None'].concat(g_cfg.getInterfaces(['board'])),
		    wanDevname = g_net.isBridge ? "" : g_net.wan.getDevname(),
		    disabledNetconf = g_cfg.netconf.find({ status: "enabled", up: "disabled" }),
		    $disableNet = $('#disableNet'),
		    cur = 'None',
		    isSoho = g_cfg.netmode == 'soho';
		if (disabledNetconf.length == 1)
			cur = disabledNetconf[0].devname;
		else if (disabledNetconf.length == 2) {
			if (isSoho)
				cur = 'eth0/ath0';
			else
				cur = 'eth0/eth1';
		}
		$disableNet.children().remove();
		vals.sort();
		if (isSoho)
			vals.push('eth0/ath0');
		else if ($.inArray('eth1', vals) != -1)
			vals.push('eth0/eth1');
		$.each(vals, function(idx, val) {
			$disableNet.append(
				$('<option></option>')
					.prop('selected', val == cur)
					.attr('value', val)
					.text(ubnt.cfg.devname2uidevname(
						ubnt.cfg.devname2uidevname(val))));
		});
	}
},
ifaces: {
	headerId: '#headerIfaces',

	init: function() {
		var is_valid = function(devname, mtu, err) {
			var max = g_net.getMaxMtu(devname);
			var errMsg = 'MTU value must be in range [64-' + max + '].';

			var intMtu = parseInt(mtu);
			if ($.isNaN(mtu) || (intMtu < 64 || intMtu > max)) {
				err.push(errMsg);
				return false;
			}

			return true;
		};

		var tbody = $('#iface_table > tbody')[0],
            that = this,
		    errDiv = $("#ifaceErrDiv");

		$('#iface_table')
			.delegate('input.editIface', 'click', function() {
				var data = $.tmplItem(this).data;
				$(this).closest('tr').replaceWith($('#ifaceEditTmpl').tmpl([data]));
				$('#ifaceMtu').prop('title', '[64-' + g_net.getMaxMtu(data.devname) + ']');
				$(".editIface").disable();
			})
			.delegate('input.cancelIface', 'click', function() {
				var data = $.tmplItem(this).data;
				$(this).closest('tr').replaceWith($('#ifaceTmpl').tmpl([data]));
                that.render();
			})
			.delegate('input.saveIface', 'click', function() {
				var iface = $.tmplItem(this).data;
				var mtu = $('#ifaceMtu').val();

				var errors = [];
				if (is_valid(iface.devname, mtu, errors)) {
					g_cfg.netconf.findByDevname(iface.devname, true).mtu = mtu;
					iface.mtu = mtu;
					$(this).closest('tr').replaceWith($('#ifaceTmpl').tmpl([iface]));
					that.render();
				}
				showErrors(errDiv, errors);
			});

		g_net.bind('ifcAdded ifcRemoved ifcEnabled ifcDisabled', $.proxy(this, "render"));
	},
	render: function() {
        var sorted = $.grep(g_cfg.netconf.objs,
        			function(obj) { return obj.enabled(); });
		sorted.sort(function(l, r) {
			var a = ubnt.cfg.devname2uidevname(l.devname);
			    b = ubnt.cfg.devname2uidevname(r.devname);
			if (a == b) return 0;
			else if (a < b) return -1;
			else return 1;
		});

		$('#iface_table > tbody > tr').remove();
		$('#ifaceTmpl')
			.tmpl(sorted)
			.appendTo('#iface_table');
	}
},
vlan: {
	headerId: '#headerVlan',
	init: function() {
		var tbody = $('#vlan_table > tbody')[0],
		    errDiv = $("#vlanErrDiv"),
		    that = this;

		$('#vlan_table')
			.delegate('input.enableVlan', 'change', function() {
				var vlanObj = $.tmplItem(this).data,
				    enabled = this.checked;
				vlanObj.enabled(enabled);
				g_net.enableIfc(vlanObj.getFullDevname(), enabled);
			})
			.delegate('input.editVlan', 'click', function() {
                $('#vlan_table').find('.editVlan, #vlanAdd').prop('disabled', true);
				var vlanObj = $.tmplItem(this).data;
				$(this).closest('tr').replaceWith($('#vlanEditTmpl').tmpl([vlanObj]));
				that.ifcChanged();
			})
			.delegate('input.saveVlan', 'click', function() {
				var vlanObj = $.tmplItem(this).data;
				vlanObj.comment = $('#edtVlanComment').val();
				$(this).closest('tr').replaceWith($('#vlanTmpl').tmpl([vlanObj]));
                $('#vlan_table').find('.editVlan, #vlanAdd').prop('disabled', false);
				that.ifcChanged();
			})
			.delegate('input.delVlan', 'click', function() {
				var vlanObj = $.tmplItem(this).data;
				g_net.removeVlan(vlanObj);

                if ($(this).closest('tr').find('#edtVlanComment').length > 0) {
                    $('#vlan_table').find('.editVlan, #vlanAdd').prop('disabled', false);
                }

				$(this).closest('tr').remove();
				$('#vlan_table').trigger('vlanDel', vlanObj.getFullDevname());
			});

		$('#vlanAdd').click(function() {
			// TODO: add validation
			var vlan = g_cfg.get('vlan'),
			    vlanObj = vlan.create(
				$('#vlanIfc').val(),
				$('#vlanId').val(),
				$('#vlanComment').val(),
				true),
			    errors = [];
			if (vlanObj.valid(errors)) {
				if (vlan.find({ devname: vlanObj.devname, id: vlanObj.id}).length) {
					errors.push('VLAN already exist.');
				}
				else {
					g_net.addVlan(vlanObj);
					$('#vlan_table tfoot input[type=text]').val('');
					$('#vlanTmpl')
						.tmpl(vlanObj)
						.appendTo('#vlan_table');
					$('#vlan_table').trigger('vlanAdd', vlanObj.getFullDevname());
				}
			}
			showErrors(errDiv, errors);
		});

		g_net
			.bind('ifcUsed', this.ifcChanged)
			.bind('ifcUnused', this.ifcChanged);
	},
	render: function() {
		var vlan = g_cfg.get('vlan');
		$('#vlanIfc').children().remove();
		$.each(g_cfg.getInterfaces(['board']), function(idx, devname) {
			$('#vlanIfc').append(
				$('<option></option>')
					.attr('value', devname)
					.text(ubnt.cfg.devname2uidevname(devname)));
		});

		$('#vlan_table > tbody > tr').remove();
		$('#vlanTmpl')
			.tmpl(vlan.objs)
			.appendTo('#vlan_table');
		this.ifcChanged();
	},
	ifcChanged: function() {
		$('#vlan_table > tbody > tr').each(function(idx) {
			var $this = $(this),
			    vlanDevname = $this.tmplItem().data.getFullDevname(),
			    used = $.inArray(vlanDevname, g_net.freeIfc) == -1;
			$this.find('input.enableVlan').prop('disabled', used);
			$this.find('input.delVlan').prop('disabled', used);
		});
	}
},
bridge: {
	headerId: '#headerBridge',
	init: function() {
		var that = this;

		$('#bridgeAdd').click(function() {
			// TODO: add validation
			var bridgeObj = g_net.addBridge();
			$('#bridgeTmpl')
				.tmpl(bridgeObj)
				.appendTo('#bridge_table');
			that.updateAddBridgePortSelect();
			$('#bridge_table').trigger('bridgeAdd', bridgeObj.getDevname());
		});
		$('#bridge_table')
			.delegate('input.enableBridge', 'change', function() {
				var bridgeObj = $.tmplItem(this).data,
				    enabled = this.checked;
				bridgeObj.enabled(enabled);
				g_net.enableIfc(bridgeObj.devname, enabled);
			})
			.delegate('input.enableBridgeStp', 'change', function() {
				var bridge = $.tmplItem(this).data;
				bridge.stp.enabled(this.checked);
			})
			.delegate('input.delBridge', 'click', function() {
				var bridgeObj = $.tmplItem(this).data;
				g_net.removeBridge(bridgeObj);
				$(this).closest('tr').remove();
				that.updateAddBridgePortSelect();
			})
			.delegate('select.addBridgePort', 'change', function() {
				var devname = $(this).val(),
				    bridgeObj = $.tmplItem(this).data;

				if (devname == "add") return; 

				if (g_net.addBridgePort(bridgeObj, devname) != null) {
					$(this).siblings('select.bridgePortList').append(
						$('<option></option>')
							.attr('value', devname)
							.text(ubnt.cfg.devname2uidevname(devname)));
					$(this).val('add');
					that.updateAddBridgePortSelect();
				}
			})
			.delegate('input.delBridgePort', 'click', function() {
				var bridgeObj = $.tmplItem(this).data,
				    $delItems = $(this).siblings('select.bridgePortList').children('option:selected');
				$delItems.each(function(idx, item) {
					g_net.removeBridgePort(bridgeObj, item.value);
				});
				$delItems.remove();
				that.updateAddBridgePortSelect();
			})

		g_net
			.bind('ifcUnused', function(e, devname) {
				that.updateAddBridgePortSelect();
			})
			.bind('ifcUsed', function(e, devname) {
				/* TODO: remove port from bridge
				var $delPorts = $('#bridge_table select.bridgePortList')
					.children('option[value="' + devname + '"]');
				$delPorts.each(function() {
					bridge = $.tmplItem(this).data;
					bridge.removePort(this.value);
				});
				$delPorts.remove();
				$('#bridge_table select.addBridgePort')
					.children('option[value="' + devname + '"]')
					.remove();
				*/
			});

		g_net
			.bind('ifcUsed ifcUnused', this.ifcChanged)
			.bind('ifcUsed ifcUnused ifcEnabled ifcDisabled', this.updateAddBridgePortSelect);

		$('#network')
			.bind('beforeSubmit', function() {
				if (!that.active)
					return;
				$('#bridge_table .comment').each(function() {
					var bridgeObj = $.tmplItem(this).data;
					bridgeObj.comment = $(this).val();
				});
			});
	},
	render: function() {
		$('#bridge_table > tbody > tr').remove();
		$('#bridgeTmpl')
			.tmpl(g_cfg.bridge.objs)
			.appendTo('#bridge_table');
		this.updateAddBridgePortSelect();
		this.ifcChanged();
	},
	valid: function() {
		var err = [];
		g_cfg.bridge.each(function(idx, bridgeObj) {
			if (bridgeObj.port.objs.length == 0)
				err.push(ubnt.cfg.devname2uidevname(bridgeObj.devname) + ' can not be empty.');
		});
		showErrors($('#bridgeErrDiv'), err);
		return (err.length == 0);
	},
	updateAddBridgePortSelect: function() {
		var freePorts = g_net.availableIfcsForBridgePors();
		$('#bridge_table select.addBridgePort').each(function() {
			var bridgeObj = $.tmplItem(this).data,
			    ports = $.grep(freePorts, $.proxy(bridgeObj, "canHavePort")).sort(),
			    $select = $(this);
			$select.children('option[value!="add"]').remove();
			$.each(filterEnabledIfcs(ports), function(idx, devname) {
				$select.append(
					$('<option></option>')
						.attr('value', devname)
						.text(ubnt.cfg.devname2uidevname(devname)));
			});
		});
	},
	ifcChanged: function() {
		$('#bridge_table > tbody > tr').each(function(idx) {
			var $this = $(this),
			    bridgeDevname = $this.tmplItem().data.getDevname(),
			    used = $.inArray(bridgeDevname, g_net.freeIfc) == -1;
			$this.find('input.enableBridge').prop('disabled', used);
			$this.find('input.delBridge').prop('disabled', used);
		});
	}
},
firewall: {
	headerId: '#headerFirewall',
	getTable: function() {
		return (g_cfg.netmode != "bridge") ? "iptables" : "ebtables";
	},
	getFw: function(table) {
		return g_cfg[table] ? g_cfg[table] : g_cfg.get(table);
	},
	init: function() {
		var that = this,
		    errDiv = $("#fwErrDiv");

		var addDefaultMask = function(value) {
			if (value && value.indexOf('/') == -1) {
				value += value === "0.0.0.0" ? "/0" : "/32";
			}
			return value;
		},
		    getRule = function(prefix, enabled) {
			var ruleItem = {
				devname   : $("#"+prefix+"Ifc").val(),
				proto     : "0x0800",
				ipProto   : $("#"+prefix+"Proto").val(),
				src       : addDefaultMask($("#"+prefix+"SrcIp").val()),
				sport     : $("#"+prefix+"SrcPort").val(),
				dst       : addDefaultMask($("#"+prefix+"DstIp").val()),
				dport     : $("#"+prefix+"DstPort").val(),
				src_inv   : $("#"+prefix+"NotSrcIp").prop("checked"),
				sport_inv : $("#"+prefix+"NotSrcPort").prop("checked"),
				dst_inv   : $("#"+prefix+"NotDstIp").prop("checked"),
				dport_inv : $("#"+prefix+"NotDstPort").prop("checked"),
				target    : $("#"+prefix+"Target").val(),
				comment   : $("#"+prefix+"Comment").val(),
				enabled   : enabled
			};
			return ruleItem;
		};

		$('#fw_table')
			.delegate('input.delFwRule', 'click', function() {
				var rule = $.tmplItem(this).data;
				that.fw.remove(rule);
				var base = $(this).closest('tr');
				base.add($(base).nextUntil(".fwbase")).remove();

                if (base.find('#fweTarget').length > 0) {
                    $('#fw_table').find('.editFwRule, #fwRuleAdd').prop('disabled', false);
                }

				showErrors(errDiv);
			})
			.delegate('input.editFwRule', 'click', function() {
                $('#fw_table').find('.editFwRule, #fwRuleAdd').prop('disabled', true);

				var rule = $.tmplItem(this).data;
				var base = $(this).closest('tr');
				base.add($(base).nextUntil(".fwbase")).replaceWith($('#fwEditRuleTmpl').tmpl([rule]));

				$("#fweTarget").val(rule._target);
				that.fillIfc($("#fweIfc"));
				$("#fweIfc").val(rule._devname);
				$("#fweProto").val(rule._ipProto);
				$("#fweProto").change(function() {
					$('.fwe_port').disable($(this).val() < 2);
				}).change();
			})
			.delegate('input.saveFwRule', 'click', function() {
				var oldRule = $.tmplItem(this).data;
				var rule = that.fw.create(getRule("fwe", oldRule.enabled));

				var errors = [];
				if (rule.valid(errors)) {
					that.fw.replace(oldRule, rule);
					var base = $(this).closest('tr');
					base.add($(base).nextUntil(".fwbase")).replaceWith($('#fwRuleTmpl').tmpl([rule]));

                    $('#fw_table').find('.editFwRule, #fwRuleAdd').prop('disabled', false);
				}
				showErrors(errDiv, errors);
			})
			.delegate('input.enableFwRule', 'change', function() {
				$.tmplItem(this).data.enabled(this.checked);
			});

		$('#fwRuleAdd').click(function() {
			var rule = that.fw.create(getRule("fw", true));

			var errors = [];
			if (rule.valid(errors)) {
				that.fw.add(rule);
				$('#fwRuleTmpl')
					.tmpl(rule)
					.appendTo('#fw_table');
				$('#fw_table tfoot input[type=text]').val('');
				$('#fw_table tfoot input[type=checkbox]').prop('checked', false);
				$('#fw_table tfoot select option:first-child').prop('selected', true);
			}
			showErrors(errDiv, errors);
		});

		$("#fwProto").change(function() {
			$('.fw_port').disable($(this).val() < 2);
		}).change();

		$("#fwEnable")
			.change(function() {
				var enabled = this.checked;
				g_cfg.get(that.table + ".sys.fw").enabled(enabled);
				if (enabled) {
					g_cfg.get(that.table + ".sys").enabled(true);
					g_cfg.get(that.table).enabled(true);
				}
				$("#fw_table").toggle(enabled);
			});

		g_net.bind('ifcAdded ifcRemoved ipModeChanged', $.proxy(this, "ifcChanged"));
	},
	render: function() {
		this.table = this.getTable();
		this.fw = this.getFw(this.table);
		var enabled = g_cfg.get(this.table + ".sys.fw").enabled();
		$('#fw_table > tbody > tr').remove();
		$("#fwEnable")
			.prop("checked", enabled);
		$("#fw_table").toggle(enabled);
		this.ifcChanged();
		if (this.fw)
			$('#fwRuleTmpl')
				.tmpl(this.fw.objs)
				.appendTo('#fw_table');
	},
	fillIfc: function($fwIfc) {
		var excludeIfcs = g_net.isBridge ? 
			    g_cfg.bridge.getInterfaces() : g_cfg.bridge.getPorts();
		$fwIfc.children().remove();
		$fwIfc.append($("<option></option>").attr("value", "").text("ANY"));
		$.each(g_net.interfaces, function(idx, devname) {
			if ($.inArray(devname, excludeIfcs) == -1)
				$fwIfc.append(
					$("<option></option>")
						.attr("value", devname)
						.text(ubnt.cfg.devname2uidevname(devname)));
		});
		if (!g_net.isBridge && g_net.wan.getIpMode() == 'Pppoe')
			$fwIfc.append(
				$("<option></option>")
					.attr("value", 'ppp+')
					.text(ubnt.cfg.devname2uidevname('ppp+')));
	},
	ifcChanged: function() {
		this.fillIfc($('#fwIfc'));
	}
},
routes: {
	headerId: '#headerRoutes',
	init: function() {
		var errDiv = $("#routeErrDiv"),
		    that = this;

		$('#route_table')
			.delegate('input.delRoute', 'click', function() {
				var route = $.tmplItem(this).data;
				g_cfg.route.remove(route);

                if ($(this).closest('tr').find('#edtIp').length > 0){
                    $('#route_table').find('.editRoute, #routeAdd').prop('disabled', false);
                }
				$(this).closest('tr').remove();
				showErrors(errDiv);
			})
			.delegate('input.editRoute', 'click', function() {
                $('#route_table').find('.editRoute, #routeAdd').prop('disabled', true);
				var route = $.tmplItem(this).data;
				$(this).closest('tr').replaceWith($('#routeTmplEdit').tmpl([route]));
			})
			.delegate('input.saveRoute', 'click', function() {
				var oldRoute = $.tmplItem(this).data;

				var route = g_cfg.route.create(
					$('#edtIp').val(), $('#edtNetmask').val(),
					$('#edtGateway').val(), $('#edtComment').val(), oldRoute.enabled);

				var errors = [];
				if (route.valid(errors)) {
					g_cfg.route.replace(oldRoute, route);
					$(this).closest('tr').replaceWith($('#routeTmpl').tmpl([route]));
                    $('#route_table').find('.editRoute, #routeAdd').prop('disabled', false);
				}
				showErrors(errDiv, errors);
			})
			.delegate('input.enableRoute', 'change', function() {
				$.tmplItem(this).data.enabled(this.checked);
			});

		$('#routeAdd').click(function() {
			var route = g_cfg.route.create(
				$('#rtIp').val(), $('#rtNetmask').val(),
				$('#rtGateway').val(), $('#rtComment').val(), true);

			var errors = [];
			if (route.valid(errors)) {
				g_cfg.route.add(route);
				$('#route_table tfoot input[type=text]').val('');
				$('#routeTmpl')
					.tmpl(route)
					.appendTo('#route_table');
			}
			showErrors(errDiv, errors);
		});

		$('#network')
			.bind('beforeSubmit', function() {
				if (!that.active)
					return;
				if (g_cfg.route.objs.length > 1)
					g_cfg.route.enabled(true);
			});
	},
	render: function() {
		var route = g_cfg.route;
		$('#route_table > tbody > tr').remove();
		if (route)
			$('#routeTmpl')
				.tmpl(route.objs.slice(1))
				.appendTo('#route_table');
	}
},
portForward: {
	headerId: '#headerPortFwd',
	init: function() {
		var that = this,
		    errDiv = $("#pfwdErrDiv");

		var addDefaultMask = function(value) {
			if (value && value.indexOf('/') == -1) {
				value += value === "0.0.0.0" ? "/0" : "/32";
			}
			return value;
		};

		$('#pfwd_table')
			.delegate('input.delPortFwd', 'click', function() {
				var portfwObj = $.tmplItem(this).data;
                if ($(this).closest('tr').find('#edtHost').length > 0){
                    $('#pfwd_table').find('.editPortFwd, #pfwdAdd').prop('disabled', false);
                }

				that.portfw.remove(portfwObj);
				$(this).closest('tr').remove();
				showErrors(errDiv);
			})
			.delegate('input.editPortFwd', 'click', function() {
                $('#pfwd_table').find('.editPortFwd, #pfwdAdd').prop('disabled', true);
				var portfwObj = $.tmplItem(this).data;
				$(this).closest('tr').replaceWith($('#portFwdTmplEdit').tmpl([portfwObj]));
			})
			.delegate('input.savePortFwd', 'click', function() {
				var oldPortfwObj = $.tmplItem(this).data,
				    portfwObj = that.portfw.create(
					g_net.wan.getDevname(),
					$('#edtHost').val(), $('#edtPort').val(), $('#edtProto').val(),
					addDefaultMask($('#edtSrc').val()), addDefaultMask($('#edtDst').val()), $('#edtDport').val(), 
					$('#edtComment').val(), oldPortfwObj.enabled());

				var errors = [];
				if (portfwObj.valid(errors)) {
					that.portfw.replace(oldPortfwObj, portfwObj);
					$(this).closest('tr').replaceWith($('#portFwdTmpl').tmpl([portfwObj]));
                    $('#pfwd_table').find('.editPortFwd, #pfwdAdd').prop('disabled', false);
				}
				showErrors(errDiv, errors);
			})
			.delegate('input.enablePortFwd', 'change', function() {
				$.tmplItem(this).data.enabled(this.checked);
			});

		$('#pfwdAdd').click(function() {
			var portfwObj = that.portfw.create(
				g_net.wan.getDevname(),
				$('#pfwdHost').val(), $('#pfwdPort').val(), $('#pfwdProto').val(),
				addDefaultMask($('#pfwdSrc').val()), addDefaultMask($('#pfwdDst').val()), $('#pfwdDport').val(), 
				$('#pfwdComment').val(), true);

			var errors = [];
			if (portfwObj.valid(errors)) {
				that.portfw.add(portfwObj);
				$('#pfwd_table tfoot input[type=text]').val('');
				$('#portFwdTmpl')
					.tmpl(portfwObj)
					.appendTo('#pfwd_table');
			}
			showErrors(errDiv, errors);
		});

		$('#network')
			.bind('beforeSubmit', function() {
				if (!that.active)
					return;

				g_cfg.get('iptables').enabled(true);
				g_cfg.get('iptables.sys').enabled(true);

				if (that.portfw.enabled(that.portfw.objs.length > 0)) {
					var devname = g_net.wan.getNatDevname();
					that.portfw.each(function(idx, portfwObj) {
						portfwObj.devname = devname;
					});
				}
			});
	},
	render: function() {
		this.portfw = g_cfg.get('iptables.sys.portfw');
		$('#pfwd_table > tbody > tr').remove();

		$('#portFwdTmpl')
			.tmpl(this.portfw.objs)
			.appendTo('#pfwd_table');
	}
},
aliases: {
	headerId: '#headerAliases',
	init: function() {
		var errDiv = $("#aliasErrDiv");

		$('#alias_table')
			.delegate('input.delAlias', 'click', function() {
				var alias = $.tmplItem(this).data,
				    netconfObj = g_cfg.netconf.findByDevname(alias.devname, true);
				netconfObj.alias.remove(alias.aliasObjRef);

                if ($(this).closest('tr').find('#edtAliasIp').length > 0) {
                    $('#alias_table').find('.editAlias, #aliasAdd').prop('disabled', false);
                }

				$(this).closest('tr').remove();
				showErrors(errDiv);
			})
			.delegate('input.editAlias', 'click', function() {
                $('#alias_table').find('.editAlias, #aliasAdd').prop('disabled', true);

				var alias = $.tmplItem(this).data;
				$(this).closest('tr').replaceWith($('#aliasEditTmpl').tmpl([alias]));
			})
			.delegate('input.saveAlias', 'click', function() {
				var alias = $.tmplItem(this).data,
				    oldRef = alias.aliasObjRef,
				    netconfObj = g_cfg.netconf.findByDevname(alias.devname, true);

				var newRef = netconfObj.alias.create(
					$('#edtAliasIp').val(), $('#edtAliasNetmask').val(),
					$('#edtAliasComment').val(), oldRef.enabled());

				var errors = [];
				if (newRef.valid(errors)) {
					netconfObj.alias.replace(oldRef, newRef);
					alias.aliasObjRef = newRef;
					$(this).closest('tr').replaceWith($('#aliasTmpl').tmpl([alias]));
                    $('#alias_table').find('.editAlias, #aliasAdd').prop('disabled', false);
				}
				showErrors(errDiv, errors);
			})
			.delegate('input.enableAlias', 'change', function() {
				$.tmplItem(this).data.aliasObjRef.enabled(this.checked);
			});

		$('#aliasAdd').click(function() {
			var netconfObj = g_cfg.netconf.findByDevname($('#aliasIfc').val(), true);
			if (netconfObj) {
				var alias = {
					devname: netconfObj.devname,
					aliasObjRef: netconfObj.alias.create(
						$('#aliasIp').val(), $('#aliasNetmask').val(),
						$('#aliasComment').val(), true)
				};

				var errors = [];
				if (alias.aliasObjRef.valid(errors)) {
					netconfObj.alias.add(alias.aliasObjRef);
					$('#alias_table tfoot input[type=text]').val('');
					$('#aliasTmpl')
						.tmpl(alias)
						.appendTo('#alias_table');
				}
				showErrors(errDiv, errors);
			}
		});

		g_net
			.bind('ifcUsed', $.proxy(this, "render"))
			.bind('ifcUnused', this.fillDevnames);
	},
	render: function() {
		$('#alias_table > tbody').empty();

		this.fillDevnames();
		$('#aliasTmpl')
			.tmpl(g_net.getIpAliases())
			.appendTo('#alias_table');
	},
	fillDevnames: function() {
		var ports = g_cfg.bridge ? g_cfg.bridge.getPorts() : [],
		    devnames = $.grep(g_net.interfaces, function(devname) {
				return $.inArray(devname, ports) == -1;
		    });
		$('#aliasIfc').children().remove();
		$.each(devnames, function(idx, devname) {
			$('#aliasIfc').append(
				$('<option></option>')
				.attr('value', devname)
				.text(ubnt.cfg.devname2uidevname(devname)));
		});
	}
},
wan: {
	headerId: '#headerWan',
	init: function() {
		var $wanIfc = $('#wanIfc'),
		    that = this;

		g_net.bind('ifcUsed ifcUnused ifcEnabled ifcDisabled',
				$.proxy(this, 'updateWanIfc'));
		g_net.bind('bridgePortAdded bridgePortRemoved', $.proxy(this, 'updateMaxMtuTooltip'));

		$('[name=wanIpMode]').click(function() {
			var ipMode = $(this).val();
			$('#wanSettings .initial_hide').hide();
			$('#wan' + ipMode).show();
			g_net.wan.setIpMode(ipMode, true);
			that.fillData(g_net.wan);
			g_net.trigger('ipModeChanged');
		});

		$wanIfc
			.change(function() {
				var devname = $(this).val();
				if (!g_net.advancedCfgMode() && g_cfg.netmode == "router") {
					if (devname != g_net.wan.getDevname())
						g_net.swapWanLanIfcs(devname);
				}
				else
					g_net.setWanIfc($(this).val());
				that.updateMaxMtuTooltip();
				that.updateChangeMac();
			});

		$('#wanNat').click(function() {
			$('#wanNatProtocols').toggle(this.checked);
		});

		$('#wanDmz').click(function() {
			$('#wanDmzSection').toggle(this.checked);
		});

		$('#wanChangeMac').click(function() {
			$('#wanChangeMacSection').toggle(this.checked);
		});

		$('#network')
			.bind('beforeSubmit', function() {
				if (!that.active)
					return;

				var ipMode = $('#wanSettings input:radio[name=wanIpMode]:checked').val(),
				    wan = g_net.wan,
				    natEnabled = $('#wanNat').prop('checked'),
				    dmzEnabled = $('#wanDmz').prop('checked'),
				    blockMgmt = $('#wanBlockMgmt').prop('checked'),
				    changeMacEnabled = $('#wanChangeMac').prop('checked');
				wan.setIpMode(ipMode, true);
				switch (ipMode) {
					case 'Static':
						wan.setIp(
							$('#wanIpAddr').val(),
							$('#wanIpNetmask').val());
							wan.setGateway($('#wanGateway').val());
							wan.setDns(0, $('#wanDns1').val());
							wan.setDns(1, $('#wanDns2').val());
						break;
					case 'Dhcpc':
						wan.setDhcpcFallback(
							$('#wanDhcpcFallbackIp').val(), 
							$('#wanDhcpcFallbackNetmask').val());
						break;
					case 'Pppoe':
						wan.setPppoe({
							name: $('#wanPppoeUser').val(),
							password: $('#wanPppoePassw').val(),
							pppoe_service: $('#wanPppoeService').val(),
							fallback: $('#wanPppoeFallbackIp').val(),
							fallback_netmask: $('#wanPppoeFallbackNetmask').val(),
							mtu: $('#wanPppoeMtu').val(),
							mru: $('#wanPppoeMru').val(),
							require: { mppe128: $('#wanPppoeEncryption').prop('checked') ? "enabled" : "disabled" }
						});
						break;
				}
				if (!g_net.advancedCfgMode())
					wan.setMtu($('#wanMtu').val());
				wan.natEnabled(natEnabled);
				if (natEnabled) {
					wan.natProtocolEnabled('sip', $('#wanNatSip').prop('checked'));
					wan.natProtocolEnabled('pptp', $('#wanNatPptp').prop('checked'));
					wan.natProtocolEnabled('ftp', $('#wanNatFtp').prop('checked'));
					wan.natProtocolEnabled('rtsp', $('#wanNatRtsp').prop('checked'));
				}
				wan.dmzEnabled(dmzEnabled);
				if (dmzEnabled) {
					wan.dmzObj.host = $('#wanDmzIp').val();
					wan.dmzMgmtPortsEnabled($('#wanDmzMgmt').prop('checked'));
				}
				wan.blockMgmt(blockMgmt);
				wan.netconfObj.hwaddr.enabled(changeMacEnabled);
				if (changeMacEnabled) {
					wan.netconfObj.hwaddr.mac=($('#wanMac').val());
				}
				wan.netconfObj.autoip.enabled($('#wanAutoIp').prop('checked'));
			});
	},
	render: function() {
		var adv = g_net.advancedCfgMode();
		if (adv || g_cfg.netmode == "router") {
			$('#wanIfcSection').show();
			this.updateWanIfc();
		}
		else
			$('#wanIfcSection').hide();
		$('input:radio[name=wanIpMode]')
			.filter('[value="' + g_net.wan.getIpMode() + '"]')
			.click();
		this.updateMaxMtuTooltip();
	},
	valid: function() {
		var err = [],
		    $wanSettings = $('#wanSettings');
		    ipMode = $('#wanSettings input:radio[name=wanIpMode]:checked').val(),
		    dmzEnabled = $('#wanDmz').prop('checked'),
		    changeMacEnabled = $('#wanChangeMac').prop('checked'),
		    maxMtu = g_net.wan.getMaxMtu();
		switch (ipMode) {
			case 'Static':
					validateFieds($wanSettings, [
						{ name: 'WAN IP', field: '#wanIpAddr', req: true, valid: _validateNonZeroIP },
						{ name: 'WAN Netmask', field: '#wanIpNetmask', req: true, valid: _validateNetmask },
						{ name: 'WAN Gateway IP', field: '#wanGateway', req: true, valid: _validateNonZeroIP },
						{ name: 'Primary DNS IP', field: '#wanDns1', req: true, valid: _validateNonZeroIP },
						{ name: 'Secondary DNS IP', field: '#wanDns2', req: false, valid: _validateNonZeroIP },
						{ name: 'MTU', field: '#wanMtu', req: true, min: 64, max: maxMtu }
						], err);
				break;
			case 'Dhcpc':
					validateFieds($wanSettings, [
						{ name: 'DHCP Fallback IP', field: '#wanDhcpcFallbackIp', req: true, valid: _validateNonZeroIP },
						{ name: 'DHCP Fallback Netmask', field: '#wanDhcpcFallbackNetmask', req: true, valid: _validateNetmask },
						{ name: 'MTU', field: '#wanMtu', req: true, min: 64, max: maxMtu }
						], err);
				break;
			case 'Pppoe':
					validateFieds($wanSettings, [
						{ name: 'PPPoE Fallback IP', field: '#wanPppoeFallbackIp', req: false, valid: _validateNonZeroIP },
						{ name: 'PPPoE Fallback Netmask', field: '#wanPppoeFallbackNetmask', req: false, valid: _validateNetmask },
						{ name: 'PPPoE MTU', field: '#wanPppoeMtu', req: true, min: 64, max: maxMtu },
						{ name: 'PPPoE MRU', field: '#wanPppoeMru', req: true, min: 64, max: maxMtu }
						], err);
				break;
		}
		if (dmzEnabled) {
			validateFieds($wanSettings, [
				{ name: 'DMZ IP', field: '#wanDmzIp', req: true, valid: _validateNonZeroIP }
				], err);
		}
		if (changeMacEnabled) {
			validateFieds($wanSettings, [
				{ name: 'MAC Address', field: '#wanMac', req: true, 
				  valid: function(value) {
				         	return /^([0-9A-F]{2}:?){5}[0-9A-F]{2}$/i.test(value) &&
				         		! /^(F{2}:?){5}F{2}$/i.test(value);
				         }
				}
				], err);
		}
		showErrors($wanSettings.find('.errors'), err);
		return (err.length == 0);
	},
	fillData: function(wan) {
		$('#wanIpAddr').val(wan.netconfObj.ip);
		$('#wanIpNetmask').val(wan.netconfObj.netmask);
		$('#wanGateway').val(wan.getGateway());
		$('#wanDns1').val(wan.getDns(0));
		$('#wanDns2').val(wan.getDns(1));

		$('#wanDhcpcFallbackIp').val(wan.getDhcpcFallback());
		$('#wanDhcpcFallbackNetmask').val(wan.getDhcpcFallbackNetmask());

		$('#wanPppoeUser').val(wan.getPppoe('name'));
		$('#wanPppoePassw').val(wan.getPppoe('password'));
		$('#wanPppoeService').val(wan.getPppoe('pppoe_service'));
		$('#wanPppoeFallbackIp').val(wan.getPppoe('fallback'));
		$('#wanPppoeFallbackNetmask').val(wan.getPppoe('fallback_netmask'));
		$('#wanPppoeMtu').val(wan.getPppoe('mtu'));
		$('#wanPppoeMru').val(wan.getPppoe('mru'));
		$('#wanPppoeEncryption').prop('checked', 
			wan.pppObj && wan.pppObj.require && wan.pppObj.require.mppe128 ?
				wan.pppObj.require.mppe128 == "enabled" : false);

		$('#wanMtuSection').toggle(wan.ipMode != "Pppoe" && !g_net.advancedCfgMode());
		$('#wanMtu').val(wan.netconfObj.mtu);
		$('#wanNat').prop('checked', wan.natEnabled());
		$('#wanNatProtocols').toggle(wan.natEnabled());
		$('#wanNatSip').prop('checked', wan.natProtocolEnabled('sip'));
		$('#wanNatPptp').prop('checked', wan.natProtocolEnabled('pptp'));
		$('#wanNatFtp').prop('checked', wan.natProtocolEnabled('ftp'));
		$('#wanNatRtsp').prop('checked', wan.natProtocolEnabled('rtsp'));
		$('#wanAutoIp').prop('checked', wan.netconfObj.autoip.enabled());
		$('#wanDmz').prop('checked', wan.dmzEnabled());
		$('#wanDmzSection').toggle(wan.dmzEnabled());
		$('#wanDmzMgmt').prop('checked', wan.dmzMgmtPortsEnabled());
		$('#wanDmzIp').val(wan.dmzObj && wan.dmzObj.host ? wan.dmzObj.host : "");
		$('#wanBlockMgmt').prop('checked', wan.blockMgmt());
		this.updateChangeMac();
	},
	updateWanIfc: function() {
		if (!this.active)
			return;

		var wanDevname = g_net.wan.getDevname(),
		    mlanDevname = g_net.mlan.getDevname(),
		    simpleModeRouter = !g_net.advancedCfgMode() && g_cfg.netmode == "router",
		    freeIfc = simpleModeRouter ? [] : Array.prototype.slice.call(g_net.freeIfc, 0),
		    $wanIfc = $('#wanIfc');
		freeIfc.push(wanDevname);
		if (simpleModeRouter) {
			for (var lan in g_net.lan)
				if (g_net.lan.hasOwnProperty(lan))
					freeIfc.push(lan);
		}
		else {
			// In router mode for management can be selected wan ifc
			if (wanDevname != mlanDevname)
				freeIfc.push(mlanDevname);
		}
		freeIfc.sort();
		$wanIfc.children().remove();
		$.each(filterEnabledIfcs(freeIfc), function(idx, devname) {
			var $option = $('<option></option>')
				.attr('value', devname)
				.text(ubnt.cfg.devname2uidevname(devname));
			if (devname == wanDevname)
				$option.prop('selected', true);
			$wanIfc.append($option);
		});
	},
	updateMaxMtuTooltip: function() {
		if (!this.active)
			return;
		$('#wanMtu').prop('title', '[64-' + g_net.wan.getMaxMtu() + ']');
	},
	isBridgeSelected: function() {
		return g_net.wan.getDevname().indexOf("br") == 0;
	},
	updateChangeMac: function() {
		var isBr = this.isBridgeSelected(),
		    wan = g_net.wan;
		$('#wanChangeMac').prop('disabled', isBr);
		$('#wanChangeMac').prop('checked', wan.netconfObj.hwaddr.enabled() && !isBr);
		$('#wanChangeMacSection').toggle(wan.netconfObj.hwaddr.enabled() && !isBr);
		$('#wanMac').val(wan.netconfObj.hwaddr && wan.netconfObj.hwaddr.mac ? wan.netconfObj.hwaddr.mac : "");
	}
},
lan: {
	headerId: '#headerLan',
	init: function() {
		var $lanIfc = $('#lanIfc'),
		    that = this;

		$('#lanAdd')
			.click(function() {
				var $lanIfc = $(this).siblings('#lanIfc'),
				    devname = $lanIfc.val(),
				    lan = g_net.addLan(devname);
				if (lan) {
					that.updateLanIfc();
					$('#lanTmpl')
						.tmpl(lan, that.tmplFuncs)
						.appendTo('#lanSettings')
						.find('input:radio[value="' + lan.getDhcpServer() + '"]').click();
					that.updateMaxMtuTooltip();
				}
			});

		$('#lanSettings')
			.delegate('.delLan', 'click', function() {
				var lan = $.tmplItem(this).data;
				g_net.removeLan(lan.getDevname());
				$(this).closest('.lanFields').remove();
				that.updateLanIfc();
			})
			.delegate('.lanDhcp', 'click', function() {
				var $this = $(this),
				    dhcp = $this.val(),
				    $lanFields = $this.closest('.lanFields'),
				    lan = $this.tmplItem().data;
				$lanFields.find('.initial_hide').hide();
				$lanFields.find('.lanDhcp' + dhcp).show();
				lan.setDhcpServer(dhcp);
				that.fillDhcp($lanFields, lan);
			})
			.delegate('.lanDhcpDnsProxy', 'click', function() {
				var $this = $(this);
				$this.parent().next().toggle(!this.checked);
			});
		
		g_net
			.bind('ifcUsed ifcUnused ifcEnabled ifcDisabled', $.proxy(this, 'updateLanIfc'))
			.bind('bridgePortAdded bridgePortRemoved', $.proxy(this, 'updateMaxMtuTooltip'))
			.bind('wanIfcChanged', $.proxy(this, 'render'));

		$('#network')
			.bind('beforeSubmit', function() {
				if (!that.active)
					return;

				var $lanFields = $('#lanSettings').find('.lanFields');
				$lanFields.each(function() {
					var $fields = $(this),
					    lan = $fields.tmplItem().data;
					lan.setIp(
						$fields.find('.lanIpAddr').val(),
						$fields.find('.lanIpNetmask').val());
					if (!g_net.advancedCfgMode())
						lan.setMtu($fields.find('.lanMtu').val());
					lan.upnpdEnabled($fields.find('.lanUPnP').prop('checked'));
					switch (lan.getDhcpServer()) {
						case "Enabled":
							var dnsproxyEnabled = $fields.find('.lanDhcpDnsProxy').prop('checked'),
							    dns = lan.dhcpdObj.dns;
							lan.dhcpdObj.start = $fields.find('.lanDhcpStart').val();
							lan.dhcpdObj.end = $fields.find('.lanDhcpEnd').val();
							lan.dhcpdObj.netmask = $fields.find('.lanDhcpNetmask').val();
							lan.dhcpdObj.lease_time = $fields.find('.lanDhcpLeaseTime').val();
							lan.dhcpdObj.dnsproxy = dnsproxyEnabled ? "enabled" : "disabled";
							dns.objs[0].status = dns.objs[1].status = dnsproxyEnabled ? "disabled" : "enabled";
							if (!dnsproxyEnabled) {
								dns.objs[0].server = $fields.find('.lanDhcpDns1').val();
								dns.objs[1].server = $fields.find('.lanDhcpDns2').val();
							}
							break;
						case "Relay":
							lan.dhcpRelayServerObj.devname = g_net.wan.getDevname();
							lan.dhcpRelayServerObj.ip = $fields.find('.lanDhcpSrvIp').val();
							lan.dhcpRelayClientObj.agentid = $fields.find('.lanDhcpAgent').val();
							break;
					}
				});
			});
	},
	render: function() {
		var $lanSettings = $('#lanSettings'),
		    adv = g_net.advancedCfgMode(),
		    routerMode = g_cfg.netmode == "router",
		    lans =  [];
		this.updateLanIfc();
		$lanSettings.children().remove();

		for (var devname in g_net.lan) {
			if (g_net.lan[devname].enabled()) {
				lans.push(g_net.lan[devname]);
			}
		}
		$('#lanTmpl')
			.tmpl(lans, this.tmplFuncs)
			.appendTo($lanSettings);
		$lanSettings
			.find('.lanFields').each(function() {
				var $this = $(this),
				    lan = $this.tmplItem().data;
				$this.find('input:radio[value="' + lan.getDhcpServer() + '"]').click();
			})
			.find('.lanIfcSection').toggle(adv || routerMode)
			.find('.delLan').toggle(adv);
		this.updateMaxMtuTooltip();
		$('#addLanSection').toggle(adv);
	},
	valid: function() {
		var $lanFields = $('#lanSettings').find('.lanFields'),
		    valid = true;
		$lanFields.each(function() {
			var err = [],
			    $fields = $(this),
			    lan = $fields.tmplItem().data,
			    maxMtu = lan.getMaxMtu();

			validateFieds($fields, [
				{ name: 'LAN IP address', field: '.lanIpAddr', req: true, valid: _validateNonZeroIP },
				{ name: 'LAN Netmask', field: '.lanIpNetmask', req: true, valid: _validateNetmask },
				{ name: 'LAN MTU', field: '.lanMtu', req: true, min: 64, max: maxMtu }
				], err);
			switch (lan.getDhcpServer()) {
				case "Enabled":
					var dnsproxyEnabled = $fields.find('.lanDhcpDnsProxy').prop('checked');
					validateFieds($fields, [
						{ name: 'DHCP Range Start', field: '.lanDhcpStart', req: true, valid: _validateNonZeroIP },
						{ name: 'DHCP Range End', field: '.lanDhcpEnd', req: true, valid: _validateNonZeroIP },
						{ name: 'DHCP Netmask', field: '.lanDhcpNetmask', req: true, valid: _validateNetmask },
						{ name: 'DHCP Lease Time', field: '.lanDhcpLeaseTime', req: true, min: 120, max: 172800 }
						], err);
					if (!dnsproxyEnabled) {
						validateFieds($fields, [
							{ name: 'DHCP Primary DNS IP', field: '.lanDhcpDns1', req: false, valid: _validateNonZeroIP },
							{ name: 'DHCP Secondary DNS IP', field: '.lanDhcpDns2', req: false, valid: _validateNonZeroIP }
							], err);
					}
					break;
				case "Relay":
					validateFieds($fields, [
						{ name: 'DHCP Server', field: '.lanDhcpSrvIp', req: true, valid: _validateNonZeroIP }
						], err);
					break;
			}
			showErrors($fields.find('.errors'), err);
			valid = valid && (err.length == 0);
		});
		return valid;
	},
	fillDhcp: function($fields, lan) {
		var dnsproxyEnabled = lan.getDhcpdAttr('dnsproxy') == "enabled";
		$fields.find('.lanDhcpStart').val(lan.getDhcpdAttr('start'));
		$fields.find('.lanDhcpEnd').val(lan.getDhcpdAttr('end'));
		$fields.find('.lanDhcpNetmask').val(lan.getDhcpdAttr('netmask'));
		$fields.find('.lanDhcpLeaseTime').val(lan.getDhcpdAttr('lease_time'));
		$fields.find('.lanDhcpDnsProxy').prop('checked', dnsproxyEnabled);
		$fields.find('.lanDhcpDnsSection').toggle(!dnsproxyEnabled);
		if (lan.dhcpdObj && lan.dhcpdObj.dns) {
			var dns = lan.dhcpdObj.dns;
			if (dns && dns.objs.length > 0)
				$fields.find('.lanDhcpDns1').val(dns.objs[0].server);
			if (dns && dns.objs.length > 1)
				$fields.find('.lanDhcpDns2').val(dns.objs[1].server);
		}
		if (lan.dhcpRelayServerObj)
			$fields.find('.lanDhcpSrvIp').val(lan.dhcpRelayServerObj.ip);
		if (lan.dhcpRelayClientObj)
			$fields.find('.lanDhcpAgent').val(lan.dhcpRelayClientObj.agentid);
	},
	updateLanIfc: function() {
		if (!this.active)
			return;

		var $lanIfc = $('#lanIfc');
		$lanIfc.children().remove();
		$.each(filterEnabledIfcs(g_net.freeIfc), function(idx, devname) {
			var $option = $('<option></option>')
				.attr('value', devname)
				.text(ubnt.cfg.devname2uidevname(devname));
			$lanIfc.append($option);
		});
		$lanIfc.prop('disabled', g_net.freeIfc.length == 0);
		$('#lanAdd').prop('disabled', g_net.freeIfc.length == 0);
	},
	tmplFuncs: {
		  genId: function(p) {
			return p + this.data.netconfObj.getIdx();
		  }
	},
	updateMaxMtuTooltip: function() {
		if (!this.active)
			return;
		var $lanFields = $('#lanSettings').find('.lanFields');
		$lanFields.each(function() {
			var $fields = $(this),
			    lan = $fields.tmplItem().data;
			$('.lanMtu').prop('title', '[64-' + lan.getMaxMtu() + ']');
		});
	}
},
mlan: {
	headerId: '#headerMgmt',
	init: function() {
		var $mgmtIfc = $('#mgmtIfc'),
		    that = this;

		g_net.bind('ifcUsed ifcUnused ifcEnabled ifcDisabled', this.updateMlanIfc);
		g_net.bind('bridgePortAdded bridgePortRemoved', $.proxy(this, 'updateMaxMtuTooltip'));

		$('[name=mgmtIpMode]').click(function() {
			$('#mgmtStatic').hide();
			$('#mgmtDhcpc').hide();
			$('#mgmt' + $(this).val()).show();
		});

		$mgmtIfc
			.change(function() {
				g_net.setMlanIfc($(this).val());
				that.render();
			});

		$('#mgmtVlanEnable').click(function() {
			$('#mgmtVlanIdSection').toggle(this.checked);
		});

		g_net.bind('wanIfcChanged', function() {
			$mgmtIfc.change();
		});

		$('#network')
			.bind('beforeSubmit', function() {
				if (g_net.mlan.enabled()) {
					var ipMode = $('#mgmtSettings input:radio[name=mgmtIpMode]:checked').val(),
					    mlan = g_net.mlan,
					    handleDefaultRoute = g_net.isBridge;
					mlan.setIpMode(ipMode, handleDefaultRoute);
					if (ipMode == "Static") {
						mlan.setIp(
							$('#mgmtIpAddr').val(),
							$('#mgmtIpNetmask').val());
						if (g_net.isBridge) {
							mlan.setGateway($('#mgmtGateway').val());
							mlan.setDns(0, $('#mgmtDns1').val());
							mlan.setDns(1, $('#mgmtDns2').val());
						}
					}
					else if (ipMode == "Dhcpc") {
						mlan.setDhcpcFallback(
							$('#mgmtDhcpcFallbackIp').val(), 
							$('#mgmtDhcpcFallbackNetmask').val());
					}
					if (!g_net.advancedCfgMode())
						mlan.setMtu($('#mgmtMtu').val());
					mlan.netconfObj.autoip.enabled($('#mgmtAutoIp').prop('checked'));
					if (!g_net.advancedCfgMode()) {
						g_net.setMgmtVlan(
							$('#mgmtVlanEnable').prop('checked') ? $('#mgmtVlanId').val() : '');
					}
				}
			});
	},
	render: function() {
		var adv = g_net.advancedCfgMode(),
		    enabled = g_net.mlan.enabled();
		$('#mgmtIfcSection').toggle(adv);
		if (adv)
			this.updateMlanIfc();
		$('#mgmtDetails').toggle(enabled);
		if (enabled) {
			this.fillData(g_net.mlan);
			this.updateMaxMtuTooltip();
		}
	},
	valid: function() {
		if (!g_net.mlan.enabled())
			return true;
		var err = [],
		    $mgmtSettings = $('#mgmtSettings');
		    ipMode = $('#mgmtSettings input:radio[name=mgmtIpMode]:checked').val(),
		    maxMtu = g_net.mlan.getMaxMtu();
		if (!g_net.advancedCfgMode() &&
		    $('#mgmtVlanEnable').prop('checked')) {
			g_cfg.get('vlan').create('eth0', $('#mgmtVlanId').val(), '').valid(err);
		}
		if (ipMode == "Static") {
			validateFieds($mgmtSettings, [
				{ name: 'IP address', field: '#mgmtIpAddr', req: true, valid: _validateNonZeroIP },
				{ name: 'IP Netmask', field: '#mgmtIpNetmask', req: true, valid: _validateNetmask }
				], err);
			if (g_net.isBridge) {
				validateFieds($mgmtSettings, [
					{ name: 'Gateway IP', field: '#mgmtGateway', req: true, valid: _validateNonZeroIP },
					{ name: 'Primary DNS IP', field: '#mgmtDns1', req: false, valid: _validateNonZeroIP },
					{ name: 'Secondary DNS IP', field: '#mgmtDns2', req: false, valid: _validateNonZeroIP }
					], err);
			}
		}
		else if (ipMode == "Dhcpc") {
			validateFieds($mgmtSettings, [
				{ name: 'DHCP Fallback IP', field: '#mgmtDhcpcFallbackIp', req: true, valid: _validateNonZeroIP },
				{ name: 'DHCP Fallback Netmask', field: '#mgmtDhcpcFallbackNetmask', req: true, valid: _validateNetmask }
				], err);
		}
		if (!g_net.advancedCfgMode()) {
			validateFieds($mgmtSettings, [
				{ name: 'MTU', field: '#mgmtMtu', req: true, min: 64, max: maxMtu }
				], err);
		}
		showErrors($mgmtSettings.find('.errors'), err);
		return (err.length == 0);
	},
	fillData: function(mlan) {
		var ipMode = mlan.getIpMode(),
		    adv = g_net.advancedCfgMode();
		$('input:radio[name=mgmtIpMode]')
			.filter('[value="' + ipMode + '"]')
			.click();

		$('#mgmtVlanSection').toggle(!adv);
		if (!adv) {
			var vlanId = g_net.getMgmtVlan();
			$('#mgmtVlanEnable').prop('checked', !!vlanId);
			$('#mgmtVlanIdSection').toggle(!!vlanId);
			$('#mgmtVlanId').val(vlanId);
		}

		$('#mgmtIpAddr').val(mlan.netconfObj.ip);
		$('#mgmtIpNetmask').val(mlan.netconfObj.netmask);
		if (g_net.isBridge) {
			$('#mgmtGateway').val(mlan.getGateway());
			$('#mgmtDns1').val(mlan.getDns(0));
			$('#mgmtDns2').val(mlan.getDns(1));
		}
		$('#mgmtGatewayDns').toggle(g_net.isBridge);

		$('#mgmtDhcpcFallbackIp').val(mlan.getDhcpcFallback());
		$('#mgmtDhcpcFallbackNetmask').val(mlan.getDhcpcFallbackNetmask());

		$('#mgmtMtu').val(mlan.netconfObj.mtu);
		$('#mgmtAutoIp').prop('checked', mlan.netconfObj.autoip.enabled());
	},
	updateMlanIfc: function() {
		var $mgmtIfc = $('#mgmtIfc'),
		    mlanDevname = g_net.mlan.getDevname(),
		    wanDevname = !g_net.isBridge ? g_net.wan.getDevname() :  mlanDevname,
		    freeIfc = Array.prototype.slice.call(g_net.freeIfc, 0);
		freeIfc.push(mlanDevname);
		// In router mode for management can be selected wan ifc
		if (wanDevname != mlanDevname)
			freeIfc.push(wanDevname);
		freeIfc.sort();
		$mgmtIfc.children().remove();
		$.each(filterEnabledIfcs(freeIfc), function(idx, devname) {
			var $option = $('<option></option>')
				.attr('value', devname)
				.text(ubnt.cfg.devname2uidevname(devname));
			if (devname == mlanDevname)
				$option.prop('selected', true);
			$mgmtIfc.append($option);
		});
	},
	updateMaxMtuTooltip: function() {
		if (!this.active)
			return;
		$('#mgmtMtu').prop('title', '[64-' + g_net.mlan.getMaxMtu() + ']');
	}
},
igmp: {
	headerId: '#headerIgmp',
	init: function() {
		var that = this;
		$('#igmpEnable').change(function() {
			var igmpproxy = g_cfg.igmpproxy;
			igmpproxy.enabled(this.checked);
			if (igmpproxy.upstream.devname === undefined) {
				igmpproxy.upstream.devname = g_net.wan.getDevname();
			}
			that.render();
		});

		$('#igmpUpstream').change(function() {
			g_cfg.igmpproxy.upstream.devname = $(this).val();
			that.updateIfcDropdowns();
		});
		$('#igmpDownstreamAdd').change(function() {
			var devname = $(this).val();
			if (devname == "add") return; 
			g_cfg.igmpproxy.add(g_cfg.igmpproxy.create(devname, true));
			$('#igmpDownstream').append(
				$('<option></option>')
					.attr('value', devname)
					.text(ubnt.cfg.devname2uidevname(devname)));
			$(this).val('add');
			that.updateIfcDropdowns();
		});
		$('#igmpDownstreamDel').click(function() {
			var $delItems = $('#igmpDownstream').children('option:selected');
			    devnames = [];
			$delItems.each(function(idx, item) {
				devnames.push(item.value);
			});
			$delItems.remove();
			that.removeDownstream(devnames);
			that.updateIfcDropdowns();
		});
		g_net
			.bind('ifcRemoved', function(devname) {
				if (devname === g_cfg.get('igmpproxy.upstream').devname)
					g_cfg.igmpproxy.enabled(false);
				else {
					that.removeDownstream([ devname ]);
				}
				that.render();
			})
			.bind('ifcUsed ifcUnused ifcAdded ifcRemoved', $.proxy(this, "updateIfcDropdowns"));
	},
	render: function() {
		var igmpproxy = g_cfg.get('igmpproxy'),
		    enabled = igmpproxy.enabled();
		$('#igmpEnable').prop('checked', enabled);
		$('#igmpSettings').toggle(enabled);
		if (enabled) {
			var $igmpDownstream = $('#igmpDownstream');
			this.updateIfcDropdowns();
			$igmpDownstream.children().remove();
			$.each(this.getDownstreamIfcs(), function(idx, devname) {
				$('<option></option>')
					.attr('value', devname)
					.text(ubnt.cfg.devname2uidevname(devname))
					.appendTo($igmpDownstream);
			});
		}
	},
	getDownstreamIfcs: function() {
		var ifcs = [];
		g_cfg.get('igmpproxy').each(function(idx, igmpObj) {
			ifcs.push(igmpObj.downstream.devname);
		});
		return ifcs;
	},
	getIfcsForUpstream: function() {
		var downIfcs = this.getDownstreamIfcs(),
		    bridgePorts = g_cfg.bridge.getPorts();
		return $.grep(g_net.interfaces, function(ifc) {
			return $.inArray(ifc, downIfcs) == -1 &&
			       $.inArray(ifc, bridgePorts) == -1;
		});
	},
	getIfcsForDownstream: function() {
		var ifcs = this.getIfcsForUpstream(),
		    up = g_cfg.get('igmpproxy').upstream.devname,
		    upIdx = $.inArray(up, ifcs);
		if (upIdx != -1)
			ifcs.splice(upIdx, 1);
		return ifcs;
	},
	updateIfcDropdowns: function() {
		var up = g_cfg.get('igmpproxy').upstream.devname,
		    $up = $('#igmpUpstream'),
		    $down = $('#igmpDownstreamAdd');
		$up.children().remove();
		$.each(this.getIfcsForUpstream(), function(idx, devname) {
			var $option = $('<option></option>')
				.attr('value', devname)
				.text(ubnt.cfg.devname2uidevname(devname));
			if (devname == up)
				$option.prop('selected', true);
			$up.append($option);
		});
		$down.children('option[value!="add"]').remove();
		$.each(this.getIfcsForDownstream(), function(idx, devname) {
			var $option = $('<option></option>')
				.attr('value', devname)
				.text(ubnt.cfg.devname2uidevname(devname));
			$down.append($option);
		});
	},
	removeDownstream: function(devnames) {
		var igmpproxy = g_cfg.get('igmpproxy');
		igmpproxy.objs = igmpproxy.grep(function(igmpObj) {
			return $.inArray(igmpObj.downstream.devname, devnames) == -1;
		});
	},
    valid: function() {
        var err = [];

        lans =  [];
        for (var devname in g_net.lan) {
            if (g_net.lan[devname].enabled()) {
                lans.push(g_net.lan[devname]);
            }
        }

        var igmpproxy = g_cfg.get('igmpproxy'),
            enabled = igmpproxy.enabled();

        if (enabled && lans.length == 0) {
            err.push('Multicast Routing requires at least one LAN interface configured.');
        }

        if (enabled && this.getDownstreamIfcs().length == 0) {
            err.push('Please specify Multicast Downstream interface.');
        }

        showErrors($("#igmpErrDiv"), err);

        return (err.length == 0);
    }
},
trafficShaping: {
	headerId: '#headerTShaping',
	init: function() {
		var errDiv = $("#tshapingErrDiv"),
		    that = this;

		$('#tshaping_table')
			.delegate('input.delTShaping', 'click', function() {
				var tshaperObj = $.tmplItem(this).data;
				g_cfg.tshaper.remove(tshaperObj);
                if ($(this).closest('tr').find('#edtTSOutBurst').length > 0){
                    $('#tshaping_table').find('.editTShaping, #tshapingAdd').prop('disabled', false);
                }

				$(this).closest('tr').remove();
				showErrors(errDiv);

                var btnStatus = $('#tshapingAdd').prop('disabled');
				that.fillDevnames();
                $('#tshapingAdd').prop('disabled', btnStatus);
			})
			.delegate('input.editTShaping', 'click', function() {
                $('#tshaping_table').find('.editTShaping, #tshapingAdd').prop('disabled', true);
				var tshaperObj = $.tmplItem(this).data;
				$(this).closest('tr').replaceWith($('#tshapingEditTmpl').tmpl([tshaperObj]));
			})
			.delegate('input.saveTShaping', 'click', function() {
				var oldTshaperObj = $.tmplItem(this).data;

				var tshaperObj = g_cfg.tshaper.create(
					oldTshaperObj.devname,
					$('#edtTSInEnable').prop('checked'),
					$('#edtTSInRate').val(), $('#edtTSInBurst').val(),
					$('#edtTSOutEnable').prop('checked'),
					$('#edtTSOutRate').val(), $('#edtTSOutBurst').val(),
					oldTshaperObj.enabled);

				var errors = [];
				if (tshaperObj.valid(errors)) {
					g_cfg.tshaper.replace(oldTshaperObj, tshaperObj);
					$(this).closest('tr').replaceWith($('#tshapingTmpl').tmpl([tshaperObj]));
                    $('#tshaping_table').find('.editTShaping, #tshapingAdd').prop('disabled', false);
				}
				showErrors(errDiv, errors);
			})
			.delegate('input.enableTShaping', 'change', function() {
				$.tmplItem(this).data.enabled(this.checked);
			})
			.delegate('#edtTSInEnable', 'change', function() {
				var disable = !$(this).prop('checked');
				$('#edtTSInRate').prop('disabled', disable);
				$('#edtTSInBurst').prop('disabled', disable);
			})
			.delegate('#edtTSOutEnable', 'change', function() {
				var disable = !$(this).prop('checked');
				$('#edtTSOutRate').prop('disabled', disable);
				$('#edtTSOutBurst').prop('disabled', disable);
			});

		$('#tshapingInEnable').change(function() {
			var disable = !$(this).prop('checked');
			$('#tshapingInRate').prop('disabled', disable);
			$('#tshapingInBurst').prop('disabled', disable);
		});

		$('#tshapingOutEnable').change(function() {
			var disable = !$(this).prop('checked');
			$('#tshapingOutRate').prop('disabled', disable);
			$('#tshapingOutBurst').prop('disabled', disable);
		});

		$('#tshapingAdd').click(function() {
			var tshaperObj = g_cfg.tshaper.create(
				$('#tshapingIfc').val(),
				$('#tshapingInEnable').prop('checked'),
				$('#tshapingInRate').val(), $('#tshapingInBurst').val(),
				$('#tshapingOutEnable').prop('checked'),
				$('#tshapingOutRate').val(), $('#tshapingOutBurst').val(),
				true);

			var errors = [];
			if (tshaperObj.valid(errors)) {
				g_cfg.tshaper.add(tshaperObj);
				$('#tshaping_table tfoot input[type=text]').val('');
				$('#tshapingTmpl')
					.tmpl(tshaperObj)
					.appendTo('#tshaping_table');
				that.fillDevnames();
			}
			showErrors(errDiv, errors);
		});

		$("#tshapingEnable")
			.change(function() {
				var enabled = this.checked;
				g_cfg.get('tshaper').enabled(enabled);
				$("#tshaping_table").toggle(enabled);
			});

		g_net
			.bind('ifcUsed', this.fillDevnames)
			.bind('ifcUnused', this.fillDevnames);
	},
	render: function() {
		var tshaper = g_cfg.get('tshaper'),
		    enabled = tshaper.enabled();

		$("#tshapingEnable").prop("checked", enabled);
		$('#tshaping_table > tbody').empty();
		$("#tshaping_table").toggle(enabled);

		this.fillDevnames();
		$('#tshapingTmpl')
			.tmpl(g_cfg.get('tshaper').objs)
			.appendTo('#tshaping_table');
	},
	fillDevnames: function() {
		var usedDevnames = g_cfg.getInterfaces(['tshaper']), 
		    devnames = $.grep(g_cfg.getInterfaces(['board', 'vlan']), function(ifc) {
			    return $.inArray(ifc, usedDevnames) == -1;
		    }),
		    $tshapingIfc = $('#tshapingIfc');
		devnames.sort();
		$tshapingIfc.children().remove();
		$.each(devnames, function(idx, devname) {
			$tshapingIfc.append(
				$('<option></option>')
				.attr('value', devname)
				.text(ubnt.cfg.devname2uidevname(devname)));
		});
		$('#tshapingAdd').prop('disabled', devnames.length == 0);
	}
},
cfgMode: {
	init: function() {
		$('#cfgmode').change(function(e) {
			var cfgmode = $(this);
			if (cfgmode.val() == 'advancedMode') {
				$('#network').trigger('beforeSubmit');
				g_net.setAdvancedCfgMode(true);
				renderViews();
			} else {
				$('#network').trigger('beforeSubmit');
				if (!g_net.simpleCfgModeApplicable()) {
					// TODO: l10n
					if (!confirm('Warning: Changing to simple mode will ' +
						'erase all current advanced ' +
						'networking settings (bridge/vlan/etc)')) {
						cfgmode.val('advancedMode');
						return;
                                        }
					// TODO: Create more sophisticated switching to simple configuration
					// mode, instead of loading default configuration
					var netEvFns = g_net.evFns;
					g_cfg = g_net.switchNetmode(g_cfg.netmode);
					// XXX workaround for r10859 - keep vlans when changing network mode
					delete g_cfg.vlan;
					g_net = ubnt.net.network(g_cfg);
					g_net.evFns = netEvFns;
				}
				g_net.setAdvancedCfgMode(false);
				renderViews();
			}
		});
	},
	render: function() {
		var adv = g_net.advancedCfgMode();
		$('#cfgmode').val(adv ? 'advancedMode' : 'simpleMode');
	}
},
threeg: {
	headerId: '#headerThreeG',
	pinRequired: false,
	init: function() {
		var that = this;
		$('#network')
			.bind('beforeSubmit', function() {
				if (!that.active)
					return;
				var threegObj = g_cfg.get('3g.1');
				if (that.pinRequired) {
					threegObj.pin.enabled(true);
					threegObj.pin.code = $('#threegPin').val();
					threegObj.pin.iccid = that.iccid;
				}
				threegObj.user = $('#threegUser').val();
				threegObj.password = $('#threegPassword').val();
				threegObj.apn = $('#threegApn').val();
			});
	},
	render: function() {
		var that = this;
		    threegObj = g_cfg.get('3g.1');
		if (threegObj.pin && threegObj.pin.enabled()) {
			$('#threegPin').val(threegObj.pin.code);
			$('#threegPinSection').show().prop('Disabled', true);
		}
		else {
			$('#threegPinSection').toggle(this.pinRequired);
		}
		$('#threegUser').val(threegObj.user);
		$('#threegPassword').val(threegObj.password);
		$('#threegApn').val(threegObj.apn);
		$.ajax({
			url: "status.cgi",
			cache: false,
			dataType: "json",
			success: function (st) {
				if (st && st.threeg && st.threeg.sim_status == 2) {
					that.pinRequired = true;
					that.iccid = st.threeg.iccid;
					$('#threegPinSection')
						.show()
						.prop('Disabled', false);
				}
			},
			error: function() {}
		});
	},
	valid: function() {
		var err = [];
		if (this.pinRequired) {
			var pin = $('#threegPin').val(),
			    l = pin.length;
			if (l == 0)
				err.push(jsTranslate('SIM Card PIN Code is required.'));
			else if (l < 4 || l > 8)
				err.push(jsTranslate('SIM Card PIN Code is invalid.'));
			else {
				$.ajax({
					url: '/checkpin.cgi?gsmpin=' + pin,
					cache: false,
					dataType: 'json',
					async: false,
					timeout: 3000,
					success: function(res) {
						if (res.status != 0)
							err.push(jsTranslate(res.message));
					},
					error: function() {
						err.push(jsTranslate('Failed to validate PIN Code.'));
					}
				});
			}
		}
		showErrors($('#threegErrDiv'), err);
		return (err.length == 0);
	}
}};

function showErrors(div, errors) {
	var rows = [];
	if (errors && errors.length > 0) {
		for (var i = 0; i < errors.length; ++i) {
			if (rows.length > 0)
				rows.push('<br/>');
			if (errors[i].length > 0)
				rows.push(errors[i]);
		}
	}
	div.html(rows.join('')).toggle(rows.length > 0);
}

function filterEnabledIfcs(ifcs) {
	return $.grep(ifcs, function(ifc) {
		return !!g_cfg.netconf.findByDevname(ifc, true);
	});
}

// XXX: #2188
function quickFixForSinglePortDevices() {
	if (g_board.board.phycount > 1) return;

	$.each(g_cfg.netconf.find({ devname: 'eth1' }), function(idx, netconfObj) {
		g_cfg.netconf.remove(netconfObj);
	});

	$.each(g_cfg.bridge.objs, function(brIdx, brObj) {
		var ports = brObj.port.find({ devname: 'eth1' });
		$.each(ports, function(pIdx, portObj) {
			brObj.port.remove(portObj);
		});
	});

	var lan1VlanObjs = g_cfg.get('vlan').find({ devname: 'eth1' });
	if (lan1VlanObjs.length) {
		$.each(lan1VlanObjs, function(idx, vlanObj) {
			var vlanDev = vlanObj.getFullDevname(),
			    netconfObjs = g_cfg.get('netconf').find({ devname: vlanDev }),
			    ebtablesVlan = g_cfg.get('ebtables.sys.vlan'),
			    ebObj = ebtablesVlan.find({ devname: vlanObj.devname, id: vlanObj.id });
			g_cfg.vlan.remove(vlanObj);
			$.each(g_cfg.bridge.objs, function(brIdx, brObj) {
				var portObjs = brObj.port.find({ devname: vlanDev });
				$.each(portObjs, function(pIdx, portObj) {
					brObj.port.remove(portObj);
				});
			});
			$.each(netconfObjs, function(nIdx, netconfObj) {
				g_cfg.netconf.remove(netconfObj);
			});
			$.each(ebObj, function(idx, o) {
				ebtablesVlan.remove(o);
			});
			ebtablesVlan.enabled(ebtablesVlan.objs.length);
		});
	}
}

function initViews() {
	$.each(views, function(idx, view) {
		view.init();
	});
}

function renderViews() {
	var viewDef = {
		bridge: {
				simple: [ 'netmode', 'disableNet', 'mlan', 'cfgMode' ],
				advanced: [ 'netmode', 'disableNet', 'mlan', 'ifaces', 'aliases', 'vlan',
					'bridge', 'firewall', 'routes', 'trafficShaping', 'cfgMode' ]
			},
		router: {
				simple: [ 'netmode', 'disableNet', 'wan', 'lan', 'portForward', 'igmp', 'cfgMode' ],
				advanced: [ 'netmode', 'disableNet', 'wan', 'lan', 'mlan', 'ifaces',
					'aliases', 'vlan', 'bridge', 'firewall', 'routes', 'portForward', 'igmp', 'trafficShaping', 'cfgMode' ]
			},
		soho:	{
				simple: [ 'netmode', 'disableNet', 'wan', 'lan', 'portForward', 'igmp', 'cfgMode' ],
				advanced: [ 'netmode', 'disableNet', 'wan', 'lan', 'mlan', 'ifaces',
					'aliases', 'vlan', 'bridge', 'firewall', 'routes', 'portForward', 'igmp', 'trafficShaping', 'cfgMode' ]
			},
		'3g':	{
				simple: [ 'netmode', 'disableNet', 'threeg', 'lan', 'portForward', 'igmp', 'cfgMode' ],
				advanced: [ 'netmode', 'disableNet', 'threeg', 'lan', 'mlan', 'ifaces',
					'aliases', 'vlan', 'bridge', 'firewall', 'routes', 'portForward', 'igmp', 'trafficShaping', 'cfgMode' ]
			}
	    },
	    activeViews = viewDef[g_cfg.netmode][g_net.advancedCfgMode() ? 'advanced' : 'simple'],
	    collapsedViews = ($.cookie('netCollapsed') || '#headerIfaces,#headerVlan,#headerBridge,#headerFirewall,#headerRoutes,#headerPortFwd,#headerAliases,#headerIgmp,#headerTShaping').split(',');

	for (var viewName in views) {
		var active = $.inArray(viewName, activeViews) != -1,
		    view =  views[viewName];
		view.active = active;
		if (view.headerId) {
			var isCollapsed = $.inArray(view.headerId, collapsedViews) != -1,
			    $view = $(view.headerId);
			$view.toggle(active).next().toggle(active && !isCollapsed);
			if (isCollapsed)
				$view.addClass('active');
		}
		if (active)
			view.render();
	};

	$(".simple-only").toggle(!g_net.advancedCfgMode());
}

function validateFieds($section, defs, err) {
	// TODO: l10n for errors
	$.each(defs, function(idx, def) {
		var val = $section.find(def.field).val();
		if (def.req && !val)
			err.push(def.name + " can not be empty.");
		else if (val) {
			if (def.valid && !def.valid(val))
				err.push(def.name + " is invalid.");
			else if (def.min || def.max) {
				var ival = parseInt(val);
				if ((def.min && ival < def.min) || (def.max && ival > def.max))
					err.push(def.name + " is out of range [" + def.min + "-" + def.max + "].");
			}
		}
	});
}

function viewsValid() {
	var valid = true,
	    $errors;
	$.each(views, function(idx, view) {
		if (view.active && view.valid) {
			if (!view.valid()) {
				valid = false;
				$(view.headerId).removeClass('active').next().show();
			}
		}
	});
	$errors = $('.errors:visible');
	if (!valid && $errors.length)
		$('html,body').scrollTop($errors.offset().top);
	return valid;
}

function getConfiguration() {
	$.ajax({
		url : 'getcfg.sh',
		data : '.',
		type : 'GET',
		dataType : 'text',

		success : function(rxcfg) {
			g_cfg = ubnt.cfg.parse(rxcfg);
			quickFixForSinglePortDevices();
			g_net = ubnt.net.network(g_cfg);
			$('#netmode').val(g_cfg.netmode);
			initViews();
			renderViews();
		},
		error : function(xhr, status) {
			// TODO: handle error while receiving configuration
			alert('Failed!');
		}
	});
}

$(document).ready(function(){

	// Section toggling
	//$(".toggle_container").hide(); 
	$("h2.trigger").click(function(){
		var collapsed = "";
		$(this).toggleClass("active").next().slideToggle("fast");
		$('.active').each(function() {
			collapsed = collapsed + (collapsed ? ',#' : '#') + this.id;
		});
		$.cookie('netCollapsed', collapsed);
		return false;
	});

	$('.pwd').attr('autocomplete', 'off').passwd({
		label :  jsTranslate("Show"),
		migrate_attrs : [ 'maxLength' ]
	}).next().next().css('text-align', 'left');
        
	$.ajax({
		url : 'getboardinfo.sh',
		type : 'GET',
		dataType : 'text',

		success : function(rxboardinfo) {
			g_board = ubnt.cfg.parse(rxboardinfo);
			getConfiguration();
		},
		error : function(xhr, status) {
			// TODO: handle error while receiving configuration
			alert('Failed!');
		}
	});

	$('#show_cfg').click(function() {
		$('#network').trigger('beforeSubmit');
		g_net.updateStatuses();
		$('#debug_cfg').html(ubnt.cfg.toCfg(g_cfg).join('<br />'));
	});

	$('#network').submit(function() {
		try {
			$('#network').trigger('beforeSubmit');
			if (viewsValid()) {
				g_net.updateStatuses();
				$('#network_data').val(ubnt.cfg.toCfg(g_cfg).join('\r\n'));
				return true;
			}
		}
		catch (e) {
			alert('An error has occurred: ' + e.message);
		}
		return false;
	});

	fwUpdateCheck(false, fw_check);
});

