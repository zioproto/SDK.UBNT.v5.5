(function() {

	var ubnt = this.ubnt || (this.ubnt = {}),
	    net = ubnt.net || (ubnt.net = {});

	net.network = function(config) {
		var netitem = function(netconfObj) {
			return {
				netconfObj: netconfObj,
				enabled: function() {
					if (arguments[0] !== undefined) {
						this.netconfObj.enabled(arguments[0]);
					}
					return this.netconfObj.enabled();
				},
				getDevname: function() {
					return this.netconfObj.getDevname();
				},
				getNatDevname: function() {
					return this.ipMode === 'Pppoe' ? 'ppp+' : netconfObj.devname;
				},
				setDevname: function(devname) {
					this.netconfObj.devname = devname;
					if (this.dhcpcObj)
						this.dhcpcObj.devname = devname;
					if (this.pppObj)
						this.pppObj.devname = devname;
					if (this.dhcpdObj)
						this.dhcpdObj.devname = devname;
					if (this.dhcpRelayClientObj)
						this.dhcpRelayClientObj.devname = devname;
					this.updateNatDevname();
				},
				updateNatDevname: function() {
					if (this.masqObj)
						this.masqObj.devname = this.getNatDevname();
					if (this.dmzObj)
						this.dmzObj.devname = this.getNatDevname();
					if (this.mgmtObj)
						this.mgmtObj.devname = this.getNatDevname();
				},
				getIpMode: function() {
					var dhcpc = config.get('dhcpc'),
					    ppp = config.get('ppp');
					
					if (!this.dhcpcObj)
						this.dhcpcObj = dhcpc.findByDevname(netconfObj.devname);

					if (!this.pppObj)
						this.pppObj = ppp.findByDevname(netconfObj.devname);

					this.ipMode = 'Static';

					if (dhcpc.enabled() && this.dhcpcObj && this.dhcpcObj.enabled())
						this.ipMode = 'Dhcpc';
					else if (ppp.enabled() && this.pppObj && this.pppObj.enabled())
						this.ipMode = 'Pppoe';
					return this.ipMode;
				},
				setIpMode: function(mode, handleDefaultRoute) {
					var tcpmss = config.get('iptables.sys.tcpmss', false);
					this.ipMode = mode;
					switch (mode) {
						case 'Static':
							if (this.dhcpcObj !== undefined && this.dhcpcObj.enabled())
								this.dhcpcObj.enabled(false);
							if (this.pppObj !== undefined && this.pppObj.enabled()) {
								this.pppObj.enabled(false);
								config.get('ppp').enabled(false);
							}
							if (tcpmss)
								tcpmss.enabled(false);
							break;
						case 'Dhcpc':
							var dhcpc = config.get('dhcpc');
							dhcpc.enabled(true);
							if (this.dhcpcObj === undefined) {
								var dhcpcObj = dhcpc.find({ devname:netconfObj.devname });
								if (dhcpcObj.length > 0) {
									this.dhcpcObj = dhcpcObj[0];
									this.dhcpcObj.devname = netconfObj.devname;
								}
								else {
									this.dhcpcObj = dhcpc.create(
												netconfObj.devname, 
												'192.168.1.20', 
												'255.255.255.0', 
												true);
									dhcpc.add(this.dhcpcObj);
								}
							}
							this.dhcpcObj.enabled(true);
							this.netconfObj.ip = "0.0.0.0";
							if (this.pppObj !== undefined && this.pppObj.enabled()) {
								this.pppObj.enabled(false);
								config.get('ppp').enabled(false);
							}
							if (handleDefaultRoute === true)
								config.get('route.1').enabled(false);
							if (tcpmss)
								tcpmss.enabled(false);
							break;
						case 'Pppoe':
							var ppp = config.get('ppp');
							ppp.enabled(true);
							if (this.pppObj === undefined) {
								var pppObj = ppp.find({ });
								if (pppObj.length > 0) {
									this.pppObj = pppObj[0];
									this.pppObj.devname = netconfObj.devname;
								}
								else {
									this.pppObj = ppp.create(
										netconfObj.devname, 
										"", //user
										"", // psw
										"", // service
										false, // mppe
										1492, // mtu
										1492, // mru
										true // enabled
									);
									ppp.add(this.pppObj);
								}
							}
							else
								this.pppObj.enabled(true);

							config.get('iptables.sys.tcpmss').enabled(true);

							netconfObj.ip = "0.0.0.0";
							netconfObj.netmask = "255.255.255.255";

							if (this.dhcpcObj !== undefined && this.dhcpcObj.enabled())
								this.dhcpcObj.enabled(false);
							if (handleDefaultRoute === true)
								config.get('route.1').enabled(false);
							break;
					}
					this.updateNatDevname();
				},
				setIp: function(ip, netmask) {
					this.netconfObj.ip = ip;
					this.netconfObj.netmask = netmask;
				},
				getGateway: function() {
					// default gw hardcoded
					var gw = "",
					    rObj = config.get('route.1');
					if (rObj.ip === "0.0.0.0") {
						gw = rObj.gateway;
					}
					return gw;
				},
				setGateway: function(gw) {
					var r = config.get('route'),
					    rObj = config.route.create("0.0.0.0", "0", gw, "", true);
					rObj.devname = this.netconfObj.devname;
					r.replace(0, rObj);
					r.enabled(true);
				},
				getDns: function(idx) {
					return config.get('resolv.nameserver.' + (idx+1)).ip;
				},
				setDns: function(idx, dns) {
					var ns = config.get('resolv.nameserver'),
					    nsObj = ns.create(dns, dns.length > 0);
					ns.replace(idx, nsObj);
					ns.enabled(true);
				},
				getDhcpcFallback: function() {
					return this.dhcpcObj && this.dhcpcObj.fallback ?
							this.dhcpcObj.fallback : 
								(config.netmode == "bridge" ? "192.168.1.20" : "192.168.10.1");
				},
				getDhcpcFallbackNetmask: function() {
					return this.dhcpcObj && this.dhcpcObj.fallback_netmask ?
							this.dhcpcObj.fallback_netmask : "255.255.255.0";
				},
				setDhcpcFallback: function(ip, netmask) {
					this.dhcpcObj.fallback = ip;
					this.dhcpcObj.fallback_netmask = netmask;
				},
				getPppoe: function(attr) {
					if (this.pppObj && this.pppObj[attr])
						return this.pppObj[attr];
					else
						return { 
							fallback: "192.168.10.1",
							fallback_netmask: "255.255.255.0",
							mtu: 1492,
							mru: 1492
						}[attr];
				},
				setPppoe: function(options) {
					$.extend(this.pppObj, options);
				},
				natEnabled: function() {
					var masq = config.get('iptables.sys.masq');

					if (!this.masqObj)
						this.masqObj = masq.findByDevname(this.getNatDevname());
					if (arguments[0] !== undefined) {
						if (this.masqObj) {
							this.masqObj.enabled(arguments[0]);
						}
						else {
							if (arguments[0])
								this.masqObj = masq.add(masq.create(this.getNatDevname(), true));
						}
						if (arguments[0]) {
							config.get('iptables').enabled(true);
							config.get('iptables.sys').enabled(true);
							masq.enabled(true);
						}
						else {
							var allDisabled = true;
							masq.each(true, function(idx, masqObj) {
								allDisabled = false;
								return false;
							});
							if (allDisabled)
								masq.enabled(false);
						}
					}
					else {
						return masq.enabled() && !!this.masqObj && this.masqObj.enabled();
					}
				},
				natProtocolEnabled: function(protocol, enable) {
					var modbl = config.get('system.modules.blacklist'),
					    expr = "ip_.*_" + protocol,
					    blObj = modbl.find({ expr: expr });
					if (enable != undefined) {
						if (!blObj.length) {
							if (!enable)
								blObj.push(
									modbl.add(
										modbl.create(expr, true)));
						}
						else
							blObj[0].enabled(!enable);
						if (!enable)
							modbl.enabled(true);
						else {
							var allDisabled = true;
							modbl.each(true, function(idx, obj) {
								allDisabled = false;
								return false;
							});
							if (allDisabled)
								modbl.enabled(false);
						}
					}
					return !(modbl.enabled() &&
					         blObj.length && blObj[0].enabled());
				},
				getDhcpServer: function() {
					var dhcpd = config.get('dhcpd'),
					    dhcpRelay = config.get('dhcprelay'),
					    dhcpRelayClient = config.get('dhcprelay.client'),
					    dhcpRelayServer = config.get('dhcprelay.server'),
					    dhcpdObj = dhcpd.find({ devname: netconfObj.devname }),
					    dhcpRelayClientObj = dhcpRelayClient.find({ devname: netconfObj.devname });

					this.dhcpd = dhcpd;
					this.dhcpRelay = dhcpRelay;
					this.dhcpRelayClient = dhcpRelayClient;
					this.dhcpRelayServer = dhcpRelayServer;
					if (dhcpdObj != undefined && dhcpdObj.length)
						this.dhcpdObj = dhcpdObj[0];
					if (dhcpRelayClientObj != undefined && dhcpRelayClientObj.length)
						this.dhcpRelayClientObj = dhcpRelayClientObj[0];

					if (dhcpd.enabled() && this.dhcpdObj && this.dhcpdObj.enabled()) {
						return "Enabled";
					}
					else if (dhcpRelay.enabled() && this.dhcpRelayClientObj && this.dhcpRelayClientObj.enabled()) {
						// XXX: server idx = client idx
						this.dhcpRelayServerObj = dhcpRelayServer.objs[this.dhcpRelayClientObj.getIdx()];
						return "Relay";
					}
					else {
						return "Disabled";
					}
				},
				setDhcpServer: function(mode) {
					switch (mode) {
					case "Disabled":
						if (this.dhcpdObj)
							this.dhcpdObj.enabled(false);
						if (this.dhcpRelayClientObj)
							this.dhcpRelayClientObj.enabled(false);
						if (this.dhcpRelayServerObj)
							this.dhcpRelayServerObj.enabled(false);
						break;
					case "Enabled":
						this.dhcpd.enabled(true);
						if (this.dhcpRelayClientObj)
							this.dhcpRelayClientObj.enabled(false);
						if (this.dhcpRelayServerObj)
							this.dhcpRelayServerObj.enabled(false);
						if (this.dhcpdObj == undefined) {
							this.dhcpdObj = this.dhcpd.create(netconfObj.devname, true);
							this.dhcpd.add(this.dhcpdObj);
						}
						else {
							this.dhcpdObj.enabled(true);
						}
						break;
					case "Relay":
						if (this.dhcpdObj)
							this.dhcpdObj.enabled(false);
						this.dhcpRelay.enabled(true);
						if (this.dhcpRelayClientObj == undefined) {
							this.dhcpRelayClientObj = this.dhcpRelayClient.create(netconfObj.devname, true);
							this.dhcpRelayClient.add(this.dhcpRelayClientObj);
							this.dhcpRelayServerObj = this.dhcpRelayServer.create("", true);
							this.dhcpRelayServer.objs[this.dhcpRelayClientObj.getIdx()] = this.dhcpRelayServerObj;
						}
						else {
							this.dhcpRelayClientObj.enabled(true);
							this.dhcpRelayServerObj.enabled(true);
						}
						break;
					}
				},
				getDhcpdAttr: function(attr) {
					if (this.dhcpdObj && this.dhcpdObj[attr])
						return this.dhcpdObj[attr];
					else
						return { 
							start: "192.168.1.2",
							end: "192.168.1.254",
							netmask: "255.255.255.0",
							lease_time: "600",
							dnsproxy: "enabled"
						}[attr];
				},
				dmzEnabled: function() {
					var dmz = config.get('iptables.sys.dmz');

					if (!this.dmzObj)
						this.dmzObj = dmz.findByDevname(this.getNatDevname());
					if (arguments[0] !== undefined) {
						if (this.dmzObj) {
							this.dmzObj.enabled(arguments[0]);
						}
						else {
							if (arguments[0])
								this.dmzObj = dmz.add(dmz.create(this.getNatDevname(), true));
						}
						if (arguments[0]) {
							config.get('iptables').enabled(true);
							config.get('iptables.sys').enabled(true);
							dmz.enabled(true);
						}
						else {
							var allDisabled = true;
							dmz.each(true, function(idz, dmzObj) {
								allDisabled = false;
								return false;
							});
							if (allDisabled)
								dmz.enabled(false);
						}
					}
					else {
						return dmz.enabled() && !!this.dmzObj && this.dmzObj.enabled();
					}
				},
				dmzMgmtPortsEnabled: function() {
					if (arguments[0] !== undefined) {
						var except = this.dmzObj.except;
						except.objs = [];
						except.enabled(!arguments[0]);
						if (!arguments[0]) {
							except.add(except.create(8, 'ICMP', true));
							except.add(except.create(9, 'UDP', true));
							except.add(except.create(config.airview && config.airview.tcp_port ? config.airview.tcp_port : 18888, 'TCP', true));
							if (config.httpd && config.httpd.enabled())
								except.add(except.create(config.httpd.port ? config.httpd.port : 80, 'TCP', true));
							if (!config.sshd || config.sshd.enabled())
								except.add(except.create(config.sshd.port ? config.sshd.port : 22, 'TCP', true));
							if (config.httpd && config.httpd.https && config.httpd.https.enabled())
								except.add(except.create(config.httpd.https.port ? config.httpd.https.port : 443, 'TCP', true));
							if (config.telnetd && config.telnetd.enabled())
								except.add(except.create(config.telnetd.port ? config.telnetd.port : 23, 'TCP', true));
							if (config.snmp && config.snmp.enabled())
								except.add(except.create(161, 'UDP', true));
							if (!config.discovery || config.discovery.enabled())
								except.add(except.create(1001, 'TCP', true));
						}
					}
					else {
						return !(this.dmzObj &&
							 this.dmzObj.except && 
						         this.dmzObj.except.enabled() &&
						         this.dmzObj.except.find({ status: "enabled" }));
					}
				},
				blockMgmt: function() {
					var mgmt = config.get('iptables.sys.mgmt');

					if (!this.mgmtObj)
						this.mgmtObj = mgmt.findByDevname(this.getNatDevname());
					if (arguments[0] !== undefined) {
						if (this.mgmtObj) {
							this.mgmtObj.enabled(arguments[0]);
						}
						else {
							if (arguments[0])
								this.mgmtObj = mgmt.add(mgmt.create(this.getNatDevname(), true));
						}
						if (arguments[0]) {
							config.get('iptables').enabled(true);
							config.get('iptables.sys').enabled(true);
							mgmt.enabled(true);
						}
						else {
							var allDisabled = true;
							mgmt.each(true, function(idx, mgmtObj) {
								allDisabled = false;
								return false;
							});
							if (allDisabled)
								mgmt.enabled(false);
						}
					}
					else {
						return mgmt.enabled() && !!this.mgmtObj && this.mgmtObj.enabled();
					}
				},
				getMaxMtu: function() {
					var result = 1524,
					    ifc = this.getDevname(),
					    bridgeObj = config.bridge.findByDevname(ifc),
					    maxMtu = function _maxMtu(devname) {
						var max = 1524;
						if (devname.indexOf('eth') == 0) {
							var vlan = config.get('vlan');
							if (vlan.isVlan(devname)) {
								max = _maxMtu(vlan.getParent(devname));
							}
							else {
								var ifcIdx = parseInt(devname.substr(3)) + 1,
								    phyObj = g_board.get('board.phy.'+ifcIdx, false);
								if (phyObj)
									max = phyObj.maxmtu;
							}
						}
						else if (devname.indexOf('ath') == 0)
							max = 2024;
						return max;
					   };
					if (bridgeObj) {
						var ports = bridgeObj.getPorts();
						for (var i = 0; i < ports.length; i++) {
							var m = maxMtu(ports[i]);
							if (i == 0 || m < result)
								result = m;
						}
					}
					else
						result = maxMtu(this.netconfObj.getDevname())
					return result;
				},
				setMtu: function(mtu) {
					var devname = this.netconfObj.getDevname(),
					    setDevMtu = function _setDevMtu(devname) {
							var vlan = config.get('vlan'),
							    netconfObj = config.netconf.findByDevname(devname);
							if (vlan.isVlan(devname)) {
								max = _setDevMtu(vlan.getParent(devname));
							}
							if (netconfObj)
								netconfObj.mtu = mtu;
					    };
					setDevMtu(devname);
					if (devname.indexOf('br') == 0) {
						var brObj = config.bridge.findByDevname(devname, true);
						if (brObj) {
							brObj.port.each(true, function(idx, portObj) {
								setDevMtu(portObj.getDevname());
							});
						}
					}
				},
				upnpdEnabled: function(enable) {
					var upnpd = config.get('upnpd'),
					    listening = config.get('upnpd.listening');
					if (!this.upnpdListeningObj) {
						var listeningObj = listening.find({ ip: this.netconfObj.ip });
						if (listeningObj.length)
							this.upnpdListeningObj = listeningObj[0];
					}
					if (enable !== undefined) {
						if (enable || (!enable && this.upnpdListeningObj)) {
							if (this.upnpdListeningObj) {
								this.upnpdListeningObj.enabled(enable);
								this.upnpdListeningObj.ip = this.netconfObj.ip;
								this.upnpdListeningObj.netmask = this.netconfObj.netmask;
							}
							else
								this.upnpdListeningObj = listening.add(
										listening.create(
											this.netconfObj.ip,
											this.netconfObj.netmask,
											enable));
						}
						upnpd.enabled(listening.find({ status: 'enabled' }).length > 0);
						return enable;
					}
					return upnpd.enabled() && 
						this.upnpdListeningObj &&
						this.upnpdListeningObj.enabled();
				}
			};
		    },
		    net = {
			lan: {},
			interfaces : [],
			freeIfc: [],
			isBridge: config.netmode == "bridge",
			removeNonExistingIfc: function() {
				var sections = [ 
					'vlan',
					'dhcpc', 'dhcpd', 'dhcprelay.server', 'dhcpcrelay.client', 'dnsmasq',
					'ebtables.sys.arpnat', 'ebtables.sys.eap', 'ebtables.sys.vlan',
					'iptables.sys.portfw', 'iptables.sys.dmz', 'iptables.sys.mgmt', 'iptables.sys.masq', 
					'netconf', 'ppp', 'qos.uplink', 'qos.downlink', 'radvd', 
					'tshaper'
					],
				    ifcs = Array.prototype.slice.call(this.interfaces, 0),
				    validIfc = function(o) {
							return $.inArray(o.devname, ifcs) != -1;
				    };
				if (config.ppp && 
				    config.ppp.objs.length &&
				    config.ppp.objs[0].enabled())
					ifcs.push('ppp+');
				$.each(sections, function(idx, section) {
					var s = config.get(section, false);
					if (s) 
						s.objs = s.grep(validIfc);
				});
				if (config.bridge)
					config.bridge.each(function(idx, br) {
						if (br.port)
							br.port.objs = br.port.grep(validIfc);
					});
				$.each(['ebtables', 'iptables'], function(i, table) {
					var t = config.get(table, false);
					if (t)
						t.objs = t.grep(function(o) {
							var args = o.cmd.split(" "),
							    ifcarg = $.inArray("-i", args);
							if (ifcarg != -1)
								return $.inArray(args[ifcarg+1], ifcs) != -1;
							else
								return true;
						});
				});
			},
			removeDuplicates: function() {
				var sections = [ 'dhcpc', 'dhcpd' ];
				$.each(sections, function(idx, section) {
					var s = config.get(section, false);
					if (s && s.objs && s.objs.length > 1) {
						var foundIfcs = [],
						    result = [];
						s.each(function(idx, obj) {
							if ($.inArray(obj.devname, foundIfcs) == -1) {
								foundIfcs.push(obj.devname);
								result.push(obj);
							}
						});
						s.objs = result;
					}
				});
			},
			addIfc: function(devname) {
				var netconf = config.netconf,
				existing = netconf.findByDevname(devname);
				if (!existing) {
					var netconfObj = netconf.create(devname, "");
					netconf.add(netconfObj);
				}
				else if (!existing.enabled()) {
					existing.enabled(true);
				}
				this.interfaces.push(devname);
				this.interfaces.sort();
				this.trigger('ifcAdded', devname);
				this.unuseIfc(devname);
			},
			removeIfc: function(devname) {
				   var netconf = config.netconf,
				   existing = netconf.findByDevname(devname);
				   if (existing) {
					   var ifcIdx = $.inArray(devname, this.interfaces);
					   // Do not delete mlan and wan netconf sections to preserve ip settings
					   if (existing.role == "mlan" || existing.role == "wan")
						   existing.enabled(false);
					   else {
						   netconf.remove(existing);
					   }
					   if (ifcIdx != -1)
						   this.interfaces.splice(ifcIdx, 1);
					   this.useIfc(devname);
					   this.trigger('ifcRemoved', devname);
				   }
			},
			addVlan: function(vlanObj) {
				var vlan = config.get('vlan');
				vlan.enabled(true);
				// Add ebtables rule if parent interface is in bridge
				if ($.inArray(vlanObj.getDevname(), config.bridge.getPorts()) != -1) {
					var ebtablesVlan = config.get('ebtables.sys.vlan');
					ebtablesVlan.add($.extend({}, vlanObj));
					ebtablesVlan.enabled(true);
				}
				vlan.add(vlanObj);
				this.addIfc(vlanObj.getFullDevname());
			},
			removeVlan: function(vlanObj) {
			    	var ebtablesVlan = config.get('ebtables.sys.vlan'),
			    	    ebObj = ebtablesVlan.find({ devname: vlanObj.devname, id: vlanObj.id });
				$.each(ebObj, function(idx, o) {
					ebtablesVlan.remove(o);
				});
				ebtablesVlan.enabled(ebtablesVlan.objs.length);
				config.vlan.remove(vlanObj);
				this.removeIfc(vlanObj.getFullDevname());
			},
			addBridge: function() {
				var bridgeObj = config.bridge.add();
				this.addIfc(bridgeObj.devname);
				return bridgeObj;
			},
			removeBridge: function(bridgeObj) {
				var ports = bridgeObj.getPorts(),
				    that = this;
				$.each(ports, function(idx, port) {
					that.removeBridgePort(bridgeObj, port);
				});
				config.bridge.remove(bridgeObj);
				this.removeIfc(bridgeObj.devname);
			},
			availableIfcsForBridgePors: function() {
				var usedPorts = config.bridge.getPorts(),
				    wanDevname = !this.isBridge && this.wan.enabled() ? this.wan.getDevname() : "",
				    mlanDevname = this.mlan.enabled() ? this.mlan.getDevname() : "";
				    return $.grep(this.freeIfc, function(devname) {
					return devname.indexOf("br") != 0 &&
						$.inArray(devname, usedPorts) == -1 && 
						devname != wanDevname && devname != mlanDevname;
				    });
			},
			addBridgePort: function(bridgeObj, devname) {
				var portObj = bridgeObj.addPort(devname);
				if (portObj != null) {
					var netconfObj = config.netconf.findByDevname(devname);
					if (netconfObj) 
						netconfObj.clear();
					if (devname.indexOf('ath') != -1) {
						var arpnat = config.get('ebtables.sys.arpnat'),
						    arpnatObj = arpnat.findByDevname(devname),
						    notVlan = devname.indexOf('.') == -1;
						    
						if (arpnatObj) {
							arpnatObj.enabled(true);
						}
						else {
							arpnat.add(arpnat.create(devname, true));
						}
						if (notVlan) {
							var eap = config.get('ebtables.sys.eap'),
							    eapObj = eap.findByDevname(devname);
							if (eapObj) {
								eapObj.enabled(true);
							}
							else {
								eap.add(eap.create(devname, true));
							}
							config.get('ebtables.sys.eap').enabled(true);
						}
					}
					if (devname.indexOf('.') == -1) {
						// Add ebtables rules for vlans if parent interface added to the bridge
						var vlanObjs = config.vlan.find({ devname: devname }),
						    ebtablesVlan = config.get('ebtables.sys.vlan');
						$.each(vlanObjs, function(idx, vlanObj) {
							ebtablesVlan.add($.extend({}, vlanObj));
							ebtablesVlan.enabled(true);
						});
					}
					this.useIfc(devname, "bridge_port");
					this.trigger('bridgePortAdded', devname);
					return portObj;
				}
			},
			removeBridgePort: function(bridgeObj, devname) {
				if (devname.indexOf('ath') != -1) {
					var arpnat = config.get('ebtables.sys.arpnat'),
					    arpnatObj = arpnat.findByDevname(devname),
					    eap = config.get('ebtables.sys.eap'),
					    eapObj = eap.findByDevname(devname);
					if (arpnatObj) {
						arpnat.remove(arpnatObj);
					}
					if (eapObj) {
						eap.remove(eapObj);
						eap.enabled(eap.objs.length);
					}
				}
				if (devname.indexOf('.') == -1) {
					// Remove ebtables rules for vlans if parent interface removed from the bridge
					var vlanObjs = config.vlan.find({ devname: devname }),
					    ebtablesVlan = config.get('ebtables.sys.vlan');
					$.each(vlanObjs, function(idx, vlanObj) {
						var ebObj = ebtablesVlan.find({ devname: vlanObj.devname, id: vlanObj.id });
						$.each(ebObj, function(idx, o) {
							ebtablesVlan.remove(o);
						});
						ebtablesVlan.enabled(ebtablesVlan.objs.length);
					});
				}
				bridgeObj.removePort(devname);
				this.unuseIfc(devname);
				this.trigger('bridgePortRemoved', devname);
			},
			addLan: function(devname) {
				var netconfObj = config.netconf.findByDevname(devname);
				if (netconfObj) {
					var lan = netitem(netconfObj);
					net.lan[devname] = lan;
					this.useIfc(devname, "lan");
					return lan;
				}
			},
			removeLan: function(devname) {
				var lan = net.lan[devname];
				lan.setDhcpServer("Disabled");
				lan.netconfObj.role = "";
				lan.netconfObj.clear();
				delete net.lan[devname];
				this.unuseIfc(devname);
			},
			setWanIfc: function(devname) {
				var prevDevname = this.wan.getDevname();
				if (prevDevname != devname) {
					var prevNetconfObjs = config.netconf.find({
									devname: prevDevname,
									role: "" }),
					    currNetconfObjs = config.netconf.find({
									devname: devname,
									role: "" }),
					    alias = this.wan.netconfObj.alias;
					this.wan.setDevname(devname);
					// There is netconf section dedicated for the wan.
					// When the ifc is assigned to the wan section, 
					// the other one has to be disabled.
					if (currNetconfObjs.length) {
						currNetconfObjs[0].enabled(false);
						this.wan.netconfObj.alias = currNetconfObjs[0].alias;
					}
					// Reenable back nectonf section for the previous selection
					if (prevNetconfObjs.length) {
						prevNetconfObjs[0].enabled(true);
						prevNetconfObjs[0].alias = alias;
					}
					else {
						var n = config.netconf.add(config.netconf.create(prevDevname, ""));
						n.alias = alias;
					}
					this.unuseIfc(prevDevname);
					this.useIfc(devname);
					this.trigger('wanIfcChanged', devname);
				}
			},
			setMlanIfc: function(devname) {
				var prevEnabled = this.mlan.enabled(),
				    nowEnabled = this.isBridge || devname != this.wan.getDevname();
				    prevDevname = this.mlan.getDevname();
				if (prevEnabled != nowEnabled || prevDevname != devname) {
					var prevNetconfObjs = config.netconf.find({
									devname: prevDevname,
									role: "" }),
					    currNetconfObjs = config.netconf.find({
									devname: devname,
									role: "" }),
					    alias = this.mlan.netconfObj.alias;
					this.mlan.enabled(nowEnabled);
					this.mlan.setDevname(devname);
					if (prevEnabled && prevDevname != devname) {
						if (prevNetconfObjs.length) {
							prevNetconfObjs[0].enabled(true);
							prevNetconfObjs[0].alias = alias;
						}
						else {
							var n = config.netconf.add(config.netconf.create(prevDevname, ""));
							n.alias = alias;
						}
						this.unuseIfc(prevDevname);
					}
					if (nowEnabled) {
						if (currNetconfObjs.length) {
							currNetconfObjs[0].enabled(false);
							this.mlan.netconfObj.alias = currNetconfObjs[0].alias;
						}
						this.useIfc(devname);
					}
					this.trigger('mlanIfcChanged', devname);
				}
				return nowEnabled;
			},
			swapWanLanIfcs: function(lanDevname) {
				var wanDevname = this.wan.getDevname();
				if (lanDevname in g_net.lan) {
					var lanObj = g_net.lan[lanDevname];
					g_net.wan.setDevname(lanDevname);
					lanObj.setDevname(wanDevname);
					delete g_net.lan[lanDevname];
					g_net.lan[wanDevname] = lanObj;
				}
				this.trigger('wanIfcChanged', lanDevname);
			},
			useIfc: function(devname, role) {
				var ifcIdx = $.inArray(devname, this.freeIfc);
				if (ifcIdx != -1) {
					if (role) {
						var netconfObj = config.netconf.find({ devname: devname, role: "" });
						if (netconfObj.length)
							netconfObj[0].role = role;
					}
					this.freeIfc.splice(ifcIdx, 1);
					this.trigger('ifcUsed', devname);
				}
			},
			unuseIfc: function(devname) {
				var netconfObjs = config.netconf.find({ devname: devname });
				$.each(netconfObjs, function(idx, netconfObj) {
					if (netconfObj.role != "mlan" && netconfObj.role != "wan")
						netconfObj.role = "";
				});
				this.freeIfc.push(devname);
				this.freeIfc.sort();
				this.trigger('ifcUnused', devname);
			},
			enableIfc: function(devname, enable) {
				var netconfObj = config.netconf.findByDevname(devname);
				if (netconfObj && netconfObj.enabled() != enable) {
					var ifcIdx = $.inArray(devname, this.freeIfc);
					netconfObj.enabled(enable);
					this.trigger(enable ? 'ifcEnabled' : 'ifcDisabled', devname);
				}
			},
			getIpAliases: function() {
				var aliases = [];
				config.netconf.each(true, function(idx, netconfObj) {
					if (netconfObj.alias) {
						netconfObj.alias.each(function(aIdx, aliasObj) {
							aliases.push({ 
								devname: netconfObj.devname,
								aliasObjRef: aliasObj
							});
						});
					}
				});
				return aliases;
			},
			switchNetmode: function(newmode) {
				var sectionsToRemove = [
					'bridge', 'dhcpc', 'dhcpd', 'dhcprelay', 'dnsmasq', 
					'ebtables', 'igmpproxy', 'iptables', 'netconf', 'ppp', 'resolv.nameserver',
					'route', 'route6', 'tshaper'
				    ],
				    newSections = ubnt.cfg.parse(ubnt.net.defaultCfg[newmode]),
				    newCfg = $.extend({}, config);
				$.each(sectionsToRemove, function(idx, section) {
					if (newCfg[section])
						delete newCfg[section];
				});
				$.extend(newCfg, newSections);

				if (newmode == "router" && !g_apmode) {
					newCfg.netconf.objs[0].devname = "ath0";
					newCfg.netconf.objs[1].devname = "eth0";
					newCfg.dhcpc.objs[0].devname = "ath0";
				}

				return newCfg;
			},
			enableIfExist: function(key, enabled) {
				var obj = config.get(key, false);
				if (obj)
					obj.enabled(enabled);
			},
			updateStatuses: function() {
				var bridgeActive = config.bridge && config.bridge.objs.length;
				this.enableIfExist('bridge', bridgeActive);
				this.enableIfExist('ebtables', bridgeActive);
				this.enableIfExist('ebtables.sys', bridgeActive);
				if (bridgeActive) {
					if (config.ebtables &&
					    config.ebtables.sys) {
						// XXX: will not work with multimple radios
						var wmode = config.get('radio.1').mode,
						    wds = config.get('wireless.1.wds').enabled(),
						    macclone = config.get('wireless.1.macclone'),
						    isSta = wmode.toLowerCase() == "managed",
						    arpnat = isSta && !wds && macclone != "enabled";
						    eap = false;
						if (isSta) {
							var wpasupplicant, device, proto;
							if ((wpasupplicant = config.get('wpasupplicant', false)) && wpasupplicant.enabled() &&
							    (device = config.get('wpasupplicant.device.1', false)) && device.enabled() &&
							    (proto = config.get('wpasupplicant.profile.1.network.1.proto.1', false)) &&
							    proto.mode != 0)
								eap = true;
						}
						else {
							var aaa, aaa1, aaaWpa;
							if ((aaa = config.get('aaa', false)) && aaa.enabled() &&
							    (aaa1 = config.get('aaa.1', false)) && aaa1.enabled() &&
							    (aaaWpa = config.get('aaa.1.wpa', false)) && aaaWpa.mode != 0)
								eap = true;
						}
						config.get('ebtables.sys.arpnat').enabled(arpnat);
						config.get('ebtables.sys.eap').enabled(eap);
					}
				}
				else {
					this.enableIfExist('ebtables', false);
				}
				// Make sure that vlan mtu < parent ifc mtu
				config.get('vlan').each(function(idx, vlanObj) {
					var parentNetconf = config.netconf.findByDevname(vlanObj.devname, true),
					    vlanNetconf = config.netconf.findByDevname(vlanObj.getFullDevname(), true);
					if (parentNetconf && vlanNetconf && parentNetconf.mtu < vlanNetconf.mtu)
						vlanNetconf.mtu = parentNetconf.mtu;
				});

				if (!this.isBridge) {
					var upnpd = config.get('upnpd'),
					    ipt = config.get('iptables'),
					    ipt_sys = config.get('iptables.sys'),
					    ipt_upnpd = config.get('iptables.sys.upnpd');
					if (upnpd.enabled()) {
						upnpd.devname = this.wan.getNatDevname();
						ipt.enabled(true);
						ipt_sys.enabled(true);
						ipt_upnpd.enabled(true);
						ipt_upnpd.devname = upnpd.devname;
					}
					else
						ipt_upnpd.enabled(false);
				}
			},
			simpleCfgModeApplicable: function() {
				var simple = true;
				if (this.getIpAliases().length)
					simple = false;
				if (simple && config.tshaper && config.tshaper.enabled())
					simple = false;
				if (simple) {
					var table = g_net.isBridge ? 'ebtables' :  'iptables',
					    fw = config.get(table + '.sys.fw', false);
					if (fw && fw.enabled())
						simple = false;
				}
				if (simple) {
					var vlan = config.get('vlan', false),
					    vlanCount = vlan ? vlan.objs.length : 0,
					    bridge = config.get('bridge', false),
					    bridgeCount = bridge ? bridge.objs.length : 0,
					    lanCount = config.netconf.find({ role: 'lan' }).length;
					switch (g_cfg.netmode) {
						case "bridge":
							if (this.getMgmtVlan()) {
								if (bridgeCount > 2 || 
								    vlanCount > config.getInterfaces(['board']))
								    simple = false;
							}
							else if (bridgeCount > 1 || vlanCount > 0)
								simple = false;
							break;
						case "router":
							if (bridgeCount > 0 || vlanCount > 0 || lanCount != 1)
								simple = false;
							break;
						case "soho":
							if (bridgeCount > 1 || vlanCount > 0 || lanCount != 1)
								simple = false;
							break;
					}
				}
				return simple;
			},
			advancedCfgMode: function() {
				var key = 'gui.network.advanced',
				    advanced = config.get(key, false),
				    advMode = false;
				if (advanced)
					advMode = advanced.enabled();
				if (!advMode && !this.simpleCfgModeApplicable())
					advMode = true;
				return advMode;
			},
			setAdvancedCfgMode: function(advanced) {
				var key = 'gui.network.advanced';
				config.get(key).enabled(advanced);
			},
			getMgmtVlan: function() {
				var id = undefined,
				    bridgeObj = config.bridge.findByDevname('br1'),
				    vlan = config.get('vlan', false);
				if (bridgeObj && vlan) {
					var ports = bridgeObj.getPorts();
					if (ports.length == config.getInterfaces(['board']).length && 
					    vlan.isVlan(ports[0])) {
						id = vlan.getId(ports[0]);
						for (var i = 1; i < ports.length; i++) {
							if (id != vlan.getId(ports[i])) {
								id = undefined;
								break;
							}
						}
					}
				}
				return id;
			},
			setMgmtVlan: function(id) {
				if (id && !config.vlan.create('eth0', id, '').valid())
					return;

				var bridgeObj = config.bridge.findByDevname('br1');
				if (bridgeObj) {
					var ports = ports = bridgeObj.getPorts();
					this.setMlanIfc('br0');
					this.removeBridge(bridgeObj);
					for (var i = 0; i < ports.length; i++) {
						this.removeVlan(config.vlan.findByFullDevname(ports[i]));
					}
				}
				if (id) {
					var ports = config.getInterfaces([ 'board' ]),
					    vlan = config.get('vlan');
					bridgeObj = this.addBridge();
					for (var i = 0; i < ports.length; i++) {
						var vlanObj = vlan.create(ports[i], id, 'For Management', true);
						this.addVlan(vlanObj);
						this.addBridgePort(bridgeObj, vlanObj.getFullDevname());
					}
				}
				this.setMlanIfc(id ? 'br1' : 'br0');
			},
			DEFAULT_MTU:         1500,
			DEFAULT_MAX_MTU:     1524,
			DEFAULT_ATH_MAX_MTU: 2024,
			getPhyMtu: function(devname) {
				phyMtu = this.DEFAULT_MAX_MTU;

				if (devname.indexOf('eth') == 0) {
					var idx = parseInt(devname.substr(3)) + 1;
					var phyObj = g_board.get('board.phy.' + idx, false);
					if (phyObj)
						phyMtu = phyObj.maxmtu;
				}
				else if (devname.indexOf('ath') == 0) {
					phyMtu = this.DEFAULT_ATH_MAX_MTU;
				}

				return phyMtu;
			},
			getMtu: function(devname) {
				var obj = config.netconf.findByDevname(devname, true);
				return parseInt(obj ? obj.mtu : this.DEFAULT_MTU);
			},
			getMaxMtu: function(devname) {
				var maxMtu = this.DEFAULT_MAX_MTU;
				var vlan = config.get('vlan');

				if (devname.indexOf('eth') == 0 || devname.indexOf('ath') == 0) {
					maxMtu = !vlan.isVlan(devname)
						? this.getPhyMtu(devname)
						: this.getMtu(vlan.getParent(devname));
				}
				else {
					var bridgeObj = config.bridge.findByDevname(devname);
					if (bridgeObj) {
						var ports = bridgeObj.getPorts();
						for (var i = 0; i < ports.length; i++) {
							var m = this.getMtu(ports[i]);
							if (vlan.isVlan(ports[i]) && this.getMaxMtu(ports[i]) < m) {
								m = this.getMaxMtu(ports[i]);
							}

							if (i == 0 || m < maxMtu)
								maxMtu = m;
						}
					}
				}

				return maxMtu;
			}
		    },
		    bridgePorts = config.bridge ? config.bridge.getPorts() : [];

		$.extend(net, ubnt.core.events);

		net.interfaces = config.getInterfaces(['board', 'bridge', 'vlan']);

		net.removeNonExistingIfc();
		net.removeDuplicates();

		$.each(net.interfaces, function(idx, devname) {
			var netconfObjs = config.netconf.find({ devname: devname }),
			    isBridgePort = $.inArray(devname, bridgePorts) != -1,
			    isFree = true;
			if (netconfObjs.length) {
				$.each(netconfObjs, function(idx, netconfObj) {
					var role = netconfObj.role;
					if (netconfObj.enabled()) {
						if (isFree) {
							switch (role) {
							case "wan":
								if (net.isBridge) {
									netconfObj.enabled(false);
								}
								else
									isFree = false;
								break;
							case "mlan":
								isFree = false;
								break;
							case "lan":
								if (net.isBridge) {
									if (isBridgePort) {
										netconfObj.role = "bridge_port";
										isFree = false;
									}
									else
										netconfObj.role = "";
								}
								else 
									isFree = false;
								break;
							case "bridge_port":
								if (isBridgePort) {
									isFree = false;
								}
								else
									netconfObj.role = "";
								break;
							default:
								if (isBridgePort) {
									netconfObj.role = "bridge_port";
									isFree = false;
								}
								else
									netconfObj.role = "";
								break;
							}
						}
						else {
							// another active netconf section?
							config.netconf.remove(netconfObj);
						}
					}
					else if (role != "wan" && role != "mlan")
						netconfObj.role = "";
				});
				if (isFree && isBridgePort)
					config.netconf.add(config.netconf.create(devname, "bridge_port"));
			}
			else {
				config.netconf.add(config.netconf.create(devname, isBridgePort ? "bridge_port" : ""));
			}
			if (isFree)
				net.freeIfc.push(devname);
		});
		
		config.netconf.each(function(idx, netconfObj) {
			var role = netconfObj.role;
			if (role === "wan" || role === "mlan") {
				if (net[role] === undefined)
					net[role] = netitem(netconfObj);
				else
					netconfObj.enabled(false);
			}
			else if (role === "lan") {
				net[role][netconfObj.getDevname()] = netitem(netconfObj);
			}
		});

		if (net.wan === undefined && !net.isBridge) {
			var wanDevname = "eth0";
			net.wan = netitem(
					config.netconf.add(
						config.netconf.create(wanDevname, "wan")));
		}
		if (net.mlan === undefined) {
			var mlanDevname = net.isBridge ? "br0" : net.wan.getDevname();
			if (net.isBridge) {
				var bridgeDevnames = config.bridge.getInterfaces();
				if (!(mlanDevname in bridgeDevnames)) {
					if (bridgeDevnames.length)
						mlanDevname = bridgeDevnames[0];
					else
						mlanDevname = "eth0";
				}
			}
			net.mlan = netitem(
					config.netconf.add(
						config.netconf.create(mlanDevname, "mlan")));
			if (!net.isBridge)
				net.mlan.enabled(false);
		}

		// TODO: Make configuration validation for selected network mode
		if (config.netmode === "bridge") {
			config.ebtables.enabled(true);
		}
		return net;
	};

	net.defaultCfg = {
		bridge: "\
netmode=bridge\n\
\n\
bridge.status=enabled\n\
bridge.1.devname=br0\n\
bridge.1.fd=1\n\
bridge.1.port.1.devname=eth0\n\
bridge.1.port.2.devname=ath0\n\
bridge.1.port.3.devname=eth1\n\
\n\
netconf.status=enabled\n\
netconf.1.status=enabled\n\
netconf.1.devname=eth0\n\
netconf.1.ip=0.0.0.0\n\
netconf.1.netmask=255.255.255.0\n\
netconf.1.up=enabled\n\
netconf.1.promisc=enabled\n\
\n\
netconf.2.status=enabled\n\
netconf.2.devname=ath0\n\
netconf.2.ip=0.0.0.0\n\
netconf.2.netmask=255.255.255.0\n\
netconf.2.up=enabled\n\
netconf.2.promisc=enabled\n\
netconf.2.allmulti=enabled\n\
\n\
netconf.3.status=enabled\n\
netconf.3.devname=br0\n\
netconf.3.ip=192.168.1.20\n\
netconf.3.netmask=255.255.255.0\n\
netconf.3.up=enabled\n\
netconf.3.role=mlan\n\
\n\
netconf.4.status=enabled\n\
netconf.4.devname=eth1\n\
netconf.4.ip=0.0.0.0\n\
netconf.4.netmask=255.255.255.0\n\
netconf.4.up=enabled\n\
netconf.4.promisc=enabled\n\
\n\
route.status=enabled\n\
route.1.status=enabled\n\
route.1.devname=br0\n\
route.1.gateway=192.168.1.1\n\
route.1.ip=0.0.0.0\n\
route.1.netmask=0\n\
\n\
ebtables.status=enabled\n\
ebtables.sys.status=enabled\n\
\n\
ebtables.sys.arpnat.status=disabled\n\
ebtables.sys.arpnat.1.status=enabled\n\
ebtables.sys.arpnat.1.devname=ath0\n\
\n\
ebtables.sys.eap.status=disabled\n\
ebtables.sys.eap.1.status=enabled\n\
ebtables.sys.eap.1.devname=ath0\n\
",
		router: "\
netmode=router\n\
\n\
netconf.status=enabled\n\
netconf.1.status=enabled\n\
netconf.1.devname=eth0\n\
netconf.1.ip=0.0.0.0\n\
netconf.1.netmask=255.255.255.0\n\
netconf.1.promisc=enabled\n\
netconf.1.up=enabled\n\
netconf.1.role=wan\n\
\n\
netconf.2.status=enabled\n\
netconf.2.devname=ath0\n\
netconf.2.ip=192.168.1.1\n\
netconf.2.netmask=255.255.255.0\n\
netconf.2.up=enabled\n\
netconf.2.promisc=enabled\n\
netconf.2.role=lan\n\
\n\
bridge.status=disabled\n\
\n\
dhcpc.status=enabled\n\
dhcpc.1.status=enabled\n\
dhcpc.1.devname=eth0\n\
dhcpc.1.fallback=192.168.10.1\n\
",
		soho: "\
netmode=soho\n\
\n\
bridge.status=enabled\n\
bridge.1.devname=br0\n\
bridge.1.fd=1\n\
bridge.1.port.1.devname=ath0\n\
bridge.1.port.2.devname=eth1\n\
\n\
netconf.status=enabled\n\
netconf.1.status=enabled\n\
netconf.1.devname=eth0\n\
netconf.1.ip=0.0.0.0\n\
netconf.1.netmask=255.255.255.0\n\
netconf.1.promisc=enabled\n\
netconf.1.up=enabled\n\
netconf.1.role=wan\n\
\n\
netconf.2.status=enabled\n\
netconf.2.devname=ath0\n\
netconf.2.ip=0.0.0.0\n\
netconf.2.netmask=255.255.255.0\n\
netconf.2.up=enabled\n\
netconf.2.promisc=enabled\n\
\n\
netconf.3.status=enabled\n\
netconf.3.devname=br0\n\
netconf.3.ip=192.168.1.1\n\
netconf.3.netmask=255.255.255.0\n\
netconf.3.up=enabled\n\
netconf.3.role=lan\n\
\n\
netconf.4.devname=eth1\n\
netconf.4.ip=0.0.0.0\n\
netconf.4.netmask=255.255.255.0\n\
netconf.4.up=enabled\n\
\n\
dhcpd.status=enabled\n\
dhcpd.1.status=enabled\n\
dhcpd.1.devname=br0\n\
dhcpd.1.dnsproxy=enabled\n\
dhcpd.1.lease_time=600\n\
dhcpd.1.netmask=255.255.255.0\n\
dhcpd.1.start=192.168.1.2\n\
dhcpd.1.end=192.168.1.254\n\
\n\
dhcpc.status=enabled\n\
dhcpc.1.status=enabled\n\
dhcpc.1.devname=eth0\n\
dhcpc.1.fallback=192.168.10.1\n\
\n\
iptables.status=enabled\n\
iptables.sys.status=enabled\n\
iptables.sys.masq.status=enabled\n\
iptables.sys.masq.1.status=enabled\n\
iptables.sys.masq.1.devname=eth0\n\
iptables.sys.mgmt.status=enabled\n\
iptables.sys.mgmt.1.status=enabled\n\
iptables.sys.mgmt.1.devname=eth0\n\
\n\
ebtables.status=enabled\n\
ebtables.sys.status=enabled\n\
\n\
ebtables.sys.arpnat.status=disabled\n\
ebtables.sys.arpnat.1.status=enabled\n\
ebtables.sys.arpnat.1.devname=ath0\n\
\n\
ebtables.sys.eap.status=disabled\n\
ebtables.sys.eap.1.status=enabled\n\
ebtables.sys.eap.1.devname=ath0\n\
",
		'3g': "\
netmode=3g\n\
\n\
bridge.status=enabled\n\
bridge.1.devname=br0\n\
bridge.1.fd=1\n\
bridge.1.port.1.devname=ath0\n\
bridge.1.port.2.devname=eth0\n\
bridge.1.port.3.devname=eth1\n\
\n\
netconf.status=enabled\n\
netconf.1.status=enabled\n\
netconf.1.devname=eth0\n\
netconf.1.ip=0.0.0.0\n\
netconf.1.netmask=255.255.255.0\n\
netconf.1.promisc=enabled\n\
netconf.1.up=enabled\n\
\n\
netconf.2.status=enabled\n\
netconf.2.devname=ath0\n\
netconf.2.ip=0.0.0.0\n\
netconf.2.netmask=255.255.255.0\n\
netconf.2.up=enabled\n\
netconf.2.promisc=enabled\n\
\n\
netconf.3.status=enabled\n\
netconf.3.devname=br0\n\
netconf.3.ip=192.168.1.1\n\
netconf.3.netmask=255.255.255.0\n\
netconf.3.up=enabled\n\
netconf.3.role=lan\n\
\n\
netconf.4.devname=eth1\n\
netconf.4.ip=0.0.0.0\n\
netconf.4.netmask=255.255.255.0\n\
netconf.4.up=enabled\n\
\n\
dhcpd.status=enabled\n\
dhcpd.1.status=enabled\n\
dhcpd.1.devname=br0\n\
dhcpd.1.dnsproxy=enabled\n\
dhcpd.1.lease_time=600\n\
dhcpd.1.netmask=255.255.255.0\n\
dhcpd.1.start=192.168.1.2\n\
dhcpd.1.end=192.168.1.254\n\
\n\
iptables.status=enabled\n\
iptables.sys.status=enabled\n\
iptables.sys.masq.status=enabled\n\
iptables.sys.masq.1.status=enabled\n\
iptables.sys.masq.1.devname=ppp+\n\
\n\
ebtables.status=enabled\n\
ebtables.sys.status=enabled\n\
\n\
ebtables.sys.arpnat.status=disabled\n\
ebtables.sys.arpnat.1.status=enabled\n\
ebtables.sys.arpnat.1.devname=ath0\n\
\n\
ebtables.sys.eap.status=disabled\n\
ebtables.sys.eap.1.status=enabled\n\
ebtables.sys.eap.1.devname=ath0\n\
\n\
3g.status=enabled\n\
3g.1.status=enabled\n\
"
	};
})();
