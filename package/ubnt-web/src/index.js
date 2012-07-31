var reload_timeout = 0;

function refreshStatus(d, textStatus, xhr) {
	if (d == null) {
		return handleError(xhr, textStatus, null);
	}

	var is_ap = (d.wireless.mode == 'ap');
	setOperationMode(d);
	update_basic(d);
	update_polling(is_ap, d.wireless.polling, d.wireless);
	update_misc(d);
	update_ifinfo(d);
	update_antenna(d.wireless.antenna);
	update_chains(d.wireless.chains);
	if (global.has_gps)
		update_gps(d.gps);
	if (document.getElementById("threeg_info"))
		update_threeg(d.threeg);
}

function handleError(xhr, textStatus, errorThrown) {
	if (xhr && xhr.status != 200 && xhr.status != 0) {
		window.location.reload();
	}
}

function reloadStatus() {
	$.ajax({
		url: "status.cgi",
		cache: false,
		dataType: "json",
		success: refreshStatus,
		error: handleError,
		complete: function(xhr, status) {
				if (reload_timeout)
					clearTimeout(reload_timeout);
				reload_timeout = setTimeout(reloadStatus, 2000);
			}
	});
	return false;
}

function getIface(s, ifname) {
	var ifaces = $.grep(s.interfaces, function(iface) { return iface.ifname == ifname; });
	return ifaces.length > 0 ? ifaces[0] : null;
}

function setOperationMode(s) {
	var wireless = s.wireless;
	var wlan = getIface(s, 'ath0');
	var airview = s.airview;
	var nmode = s.host.netrole;
	var services = s.services;
	var firewall = s.firewall;

	var is_ap = (wireless.mode == 'ap');
	var mode_string = wlan.enabled ? getModeString(is_ap, wireless.wds, wireless.aprepeater) : l10n_get('Disabled');

	var airview_status;
	if (airview.enabled==1) {
		old_mode_string = mode_string;
		mode_string = l10n_get('Spectral Analyzer');
		if (airview.status.active_connections > 0) {
			airview_status = l10n_get('Active') + ' - ' + airview.status.active_connections + ' ' + l10n_get('clients');
		} else if (airview.watchdog.seconds_to_exit > 0) {
			airview_status = l10n_get('Idle for') + ' ' + airview.watchdog.seconds_idle + l10n_get('s') + ".  " + l10n_get('Back to') + ' ' + old_mode_string + ' ' + l10n_get('in') + ' ' + airview.watchdog.seconds_to_exit + l10n_get('s');
		} else {
			airview_status = l10n_get('Switching back to') + ' ' + old_mode_string;
		}
	} else {
		airview_status = "";
	}

	var is_soho = false, is_bridge = false;
	switch (nmode) {
	case 'bridge':
		netmodestr=l10n_lang['Bridge'];
		is_bridge = true;
		break;
	case 'router':
		netmodestr=l10n_lang['Router'];
		break;
	case 'soho':
		netmodestr=l10n_lang['SOHO Router'];
		is_soho = true;
		break;
	case '3g':
		netmodestr=l10n_lang['3G Router'];
		break;
	}
	$('#netmode').text(netmodestr);
	$('#wmode').text(mode_string);
	$('#astatus').text(airview_status);
	$('#astatusinfo').toggle(airview_status != "");

	$('.apinfo').toggle(is_ap);
	$('.stainfo').toggle(!is_ap);
	$('.bridge').toggle(is_bridge);
	$('.router').toggle(!is_bridge);
	$('#wanmac').toggle(is_soho);
	$('#dhcpc_info').toggle(1 == services.dhcpc);
	$('#dhcp_leases').toggle(1 == services.dhcpd);
	$('#ppp_info').toggle(1 == services.pppoe);

	$('#a_stainfo').attr('href','stainfo.cgi?ifname=' + global['wlan_iface'] + '&sta_mac='+wireless.apmac+'&mode=ap');
	$('#a_fw').attr('href', 'fw.cgi?netmode='+nmode);
	var wd_str = (wireless.chanbw == 0) ? wireless.chwidth : wireless.chanbw;
	if (wireless.cwmmode == 1) {
        	var cwd_str = "";
        	if (global.has_ht40) cwd_str = l10n_get('Auto') + " ";
		if ((wireless.rstatus == 1) && (wireless.chanbw == 0)) 
                	cwd_str += '20 / 40';
                else 
			cwd_str += wd_str;
       		wd_str = cwd_str;
	}

	if (wlan.enabled) {
		$('#wd').text(wd_str + ' MHz');
		setExtendedChannel(wireless.opmode);
	}
	else {
		$('#wd').text('-');
		$('#ext_chan').hide();
	}

	var fwall_enabled = !is_bridge ? firewall.iptables : firewall.ebtables;
	$('#fwall').toggle(fwall_enabled == 1);
	$('.gpsinfo').toggle(global.has_gps && s.gps.status != 0);
}

function refreshContent(uri, data) {
	reloadStatus();

	if (uri.indexOf("?") > 0)
		uri = uri + '&id=';
	else
		uri = uri + '?id=';
	$("#extraFrame").load(uri+(new Date().getTime()), data, function(r,s,xhr){
		if (xhr && xhr.status != 200 && xhr.status != 0) {
			window.location.reload();
			return;
		}
		$.ready();
	});
	return false;
}

function format_rate(rate) {
	return parseInt(rate) > 0 ? '' + rate + ' Mbps' : '-';
}

function format_ccq(ccq) {
	if (!ccq) return '-';

	var dec = (ccq % 10 != 0) ? 1 : 0;
	return '' + toFixed(ccq / 10, dec) + ' %';
}

function strip_fwversion(fwversion) {
	if (fwversion.indexOf("-") > 0) {
		var ver = fwversion.split(".");
		if (ver.length > 3) {
			var v = "";
			for(i = 0; i < ver.length - 3; i++) {
				v += ver[i]+"."
			}
			v += ver[i];
			return v;
		}
	}
	return fwversion;
}

function update_basic(s) {
	var host = s.host;
	var wireless = s.wireless;

	$('#signalinfo .switchable').toggle(wireless.rstatus == 5);
	updateSignalLevel(wireless.signal, wireless.rssi, wireless.noisef,
			wireless.chwidth, wireless.rx_chainmask,
			wireless.chainrssi, wireless.chainrssiext);
	$('#hostname').text(host.hostname);
	$('#fwversion').text(strip_fwversion(host.fwversion));
	if (wireless.mode == 'ap' && wireless.hide_essid == 1)
		$('#essid_label').text(l10n_get('Hidden SSID'));

	var wlan = getIface(s, 'ath0');
	$('#essid').text(wlan.enabled ? wireless.essid : '-');

	var show_mac = (wireless.apmac != "00:00:00:00:00:00") && (wireless.mode == 'ap' || wireless.rstatus == 5);
	$('#apmac').text(show_mac ? wireless.apmac : l10n_get('Not Associated'));
	$('#frequency').text(wlan.enabled ? wireless.frequency : '-');
	$('#channel').text(wlan.enabled ? wireless.channel : '-');
	$('#txrate').text(wlan.enabled ? format_rate(wireless.txrate) : '-');
	$('#rxrate').text(wlan.enabled ? format_rate(wireless.rxrate) : '-');
	$('#count').text(wlan.enabled ? " "+wireless.count : '-');
	update_ack(s, wlan);
}

function update_ack(s, wlan) {
	var wireless = s.wireless;
	if ((wireless.polling.enabled == 0 || wireless.polling.noack == 0) && wlan.enabled) {
		var dist = (wireless.distance < 150) ? 150 : wireless.distance;
		var dist_km = toFixed(dist / 1000.00, 1);
		var dist_mi = toFixed(dist / 1609.344, 1);
		var ack_val = [dist_mi, 'miles', '(' + dist_km, 'km)'];
		$('#ack').text(ack_val.join(' '));
	}
	else {
		// PtPNoAck or WLAN interface is disabled
		$('#ack').text('-');
	}
}

function prio2text(prio) {
	var txt = l10n_get('None');
	switch (prio) {
        case 0:
        	txt = l10n_get('High');
	        break;
        case 1:
        	txt = l10n_get('Medium');
        	break;
        case 2:
        	txt = l10n_get('Low');
	        break;
        }
        return txt;
}

function update_polling(is_ap, polling, wireless) {
	if (polling.enabled > 0) {
		$('#polling').text(l10n_get('Enabled'));
		var chain_count = global['chain_count'];
		if (parseInt(chain_count) == 1 && polling.capacity <= 50)
			$('#amcborder').addClass('halfborder');
		$('.pollinfo').show();
                $('.stapollinfo').toggle(!is_ap);
		$('#airselectinfo').toggle(is_ap);
		if (polling.airselect > 0) {
			$('#airselect').text(l10n_get('Enabled'));
			$('.airselectinfo').show();
			$('#airselectinterval').text(polling.airselect_interval + " ms");
		} else {
			$('#airselect').text(l10n_get('Disabled'));
			$('.airselectinfo').hide();
		}
	} else {
		$('#polling').text(is_ap ? l10n_get('Disabled') : '-');
		$(".pollinfo").hide();
                $('.stapollinfo').hide();
		$('#airselectinfo').hide();
	}
	/* Workaround for ubnt_poll.capacity = 100% when there is no connections */
	if (wireless.apmac == "00:00:00:00:00:00" || 0 == wireless.count) {
		polling.quality = 0;
		polling.capacity = 0;
	}
	$('#pollprio').text(prio2text(polling.priority));
	$('#amq').text(polling.quality);
	$('#amc').text(polling.capacity);
	update_meter('amqbar', polling.quality, 100);
	update_meter('amcbar', polling.capacity, 100);

	if(polling.enabled > 0 && is_ap && global.has_gps) {
		switch(polling.airsync_mode) {
			case 1:
				$('#airsyncstatus').text(polling.airsync_connections + ' ' + l10n_get('Peer(s)'));
				break;
			case 2:
				if(polling.airsync_connections)
					$('#airsyncstatus').text(l10n_get('Connected'));
				else
					$('#airsyncstatus').text(l10n_get('Not Connected'));
				break;				
			case 0:
			default:
				$('#airsyncstatus').text(l10n_get('Disabled'));
				break;				
		}
		$('#airsyncinfo').show();
	} else {
		$('#airsyncinfo').hide();
	}
}

function translate_security(security) {
	var security_str = global['security'];
	if (security_str.length == 0) {
		security_str = security;
	}

	return l10n_get(security_str);
}

function update_misc(s) {
	var wlan = getIface(s, 'ath0');
	$('#security').text(wlan.enabled ? translate_security(s.wireless.security) : '-');
	$('#qos').text(s.wireless.qos);
	var uptime_str = secsToCountdown(s.host.uptime, l10n_get('day'), l10n_get('days'));
	$('#uptime').text(uptime_str);
	$('#ccq').text(format_ccq(s.wireless.ccq));
	$('#date').text(s.host.time);
        if (s.genuine && s.genuine.length) {
        	$('#logo_img').attr('src', s.genuine);
	        $('#logo_info').show();
        } else {
	        $('#logo_info').hide();
        }
}

function get_eth_str(ethstat) {
	if (ethstat && ethstat.plugged != 0) {
		if (ethstat.speed > 0) {
			var str = '' + ethstat.speed + 'Mbps';
			if (ethstat.duplex == 1)
				str += '-' + l10n_get('Full');
			else if (ethstat.duplex == 0)
				str += '-' + l10n_get('Half');
			return str;
		}
		else {
			return l10n_get('Plugged');
		}
	}
	else {
		return l10n_get('Unplugged');
	}
}

function add_ifinfo(label, value) {
	var row = [];
	row.push('<div class="row">');
	row.push('<span class="label">' + label + '</span>');
	row.push('<span class="value">' + value + '</span>');
	row.push('</div');
	$('#ifinfo').append(row.join(''));
}

function update_ifinfo(s) {
	$('#ifinfo').empty();

	var wlan = getIface(s, 'ath0');
	if (wlan)
		add_ifinfo(devname2uidevname(wlan.ifname) + ' MAC', wlan.hwaddr);

	var lans = $.grep(s.interfaces, function(iface) { return iface.ifname.indexOf("eth") == 0; });
	$.each(lans, function(idx, lan) {
		add_ifinfo(devname2uidevname(lan.ifname) + ' MAC', lan.hwaddr);
	});

	var label, value;
	if (lans.length == 1) {
		label = devname2uidevname(lans[0].ifname);
		value = get_eth_str(lans[0].status)
	}
	else if (lans.length > 1) {
		label = devname2uidevname(lans[0].ifname) + ' / ' + devname2uidevname(lans[1].ifname);
		value = get_eth_str(lans[0].status) + ' / ' + get_eth_str(lans[1].status);
	}

	add_ifinfo(label, value);
}

function update_antenna(value) {
	$('#antenna').text(l10n_get(value));
}

function update_chains(value) {
	$('#chains').text(value);
}

function showAction(select) {
	if (select.value == "")
		return;
	openPage(select.value, 700,200);
	select.selectedIndex = 0;
}

function getModeString(is_ap, is_wds, is_aprepeater) {
	var mode_string;
	if (is_ap) {
		if (is_wds)
			mode_string = is_aprepeater ? l10n_get('AP-Repeater') : l10n_get('Access Point WDS');
		else
			mode_string = l10n_get('Access Point');
	}
	else
		mode_string = is_wds ? l10n_get('Station WDS') : l10n_get('Station');

	return mode_string;
}

function setExtendedChannel(opmode) {
	var ext_ch = 0;
	if (ext_ch = /minus$/.test(opmode))
		$("#ext_chan").html('&nbsp;(' + l10n_get('Lower') + ')');
	else if (ext_ch = /plus$/.test(opmode))
		$("#ext_chan").html('&nbsp;(' + l10n_get('Upper') + ')');
	$('#ext_chan').toggle(ext_ch);
}

function get_gps_quality(gps) {
	if (gps.fix == 0)
		return 0;

	var quals = [[20, 10], [15, 20], [10, 30], [7, 40], [5, 50], [3.5, 60], [2, 70], [1.5, 80], [1, 90], [0, 100]];
	for (var i in quals) {
		if (gps.dop > quals[i][0])
			return quals[i][1];
	}

	return 0;
}

function update_gps(gps) {
	$('.gpsinfo').toggle(gps.status != 0);
	if (gps.status != 0) {
		$('#gps_status').text(l10n_get('Enabled'));

		var quality = get_gps_quality(gps);
		$('#gps_qual').text(quality);
		update_meter('gpsbar', quality, 100);

		var coordinates = (gps.fix != 0) ?
			'<a target="_blank" href=http://maps.google.com/maps?q='+ gps.lat + ',' + gps.lon + '>' + gps.lat + ' / ' + gps.lon + '</a>' :
			'- / -';
		$('#gps_coord').html(coordinates);

		var altitude = (gps.fix != 0) ? '' + Math.round(gps.alt) + ' m' : '-';
		$('#gps_alt').text(altitude);
	}
	else {
		$('#gps_status').text(l10n_get('Disabled'));
	}
}

function update_threeg(threeg) {
	if (threeg != undefined) {
		$('#threeg_info').show();
		$('#threeg_product').html('<img id="threeg_image" src="FULL_VERSION_LINK/images/' + threeg.image + '" alt="' + threeg.product + '">');
		$('#threeg_image').error(function() { $('#threeg_product').html(threeg.product); });
		switch(threeg.sim_status) {
			case 0:
				$('#threeg_signal_row').show();
				$('#threeg_status_row').hide();
				break;
			case 1:
				$('#threeg_signal_row').hide();
				$('#threeg_status_row').show();
				$('#threeg_status').text(l10n_get('Please insert SIM card'));
				break;
			case 2:
				$('#threeg_signal_row').hide();
				$('#threeg_status_row').show();
				$('#threeg_status').text(l10n_get('SIM PIN required'));
				break;
			case 3:
				$('#threeg_signal_row').hide();
				$('#threeg_status_row').show();
				$('#threeg_status').text(l10n_get('SIM Card Blocked'));
				break;
		}
		update_meter('threeg_bar', threeg.signal, 100);
	}
	else {
		$('#threeg_status_row').hide();
		$('#threeg_signal_row').hide();
		if (global.is_3g_product) {
			$('#threeg_product').text(l10n_get('Not detected'));
		}
		else {
			$('#threeg_info').hide();
		}
	}
}
