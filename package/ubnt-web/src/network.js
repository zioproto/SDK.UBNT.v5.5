var IP_regex = /^((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\.){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])$/;
function _validateIP(input) {
        return IP_regex.exec(input) != null;
}

function _validateNonZeroIP(value) {
	return _validateIP(value) && value != "0.0.0.0";
}

function _validateNetmask(input)
{
	if (!_validateIP(input))
	{
		return false;
	}
	var quad = input.split('.');
	if (quad.length != 4)
	{
		return false;
	}
	var mask_val = parseInt(quad[0])*16777216 + 65536 * parseInt(quad[1]) +
		256 * parseInt(quad[2]) + parseInt(quad[3]);
	mask_val = 256*256*256*256 - mask_val - 1;
	return (mask_val & (mask_val + 1)) == 0;
}

function changeNetworkMode(s,mode){
	var idx = s.selectedIndex;
	var newval = s.options[idx].value;
	if (newval != mode) {window.location.href="network.cgi?netmode="+newval;}
}

function validatePort(id, name, value) {
	if (!$('#' + id).is(":visible"))
		return true;
	var port = parseInt(value);
	return (port >=1 && port <= 65535);
}

function validateAirsyncIP(id, name, value) {
	var is_valid = true;
	if ($('#' + id).is(":visible") && $('#airsync_mode').val() == 2)
		is_valid = _validateNonZeroIP(value);
	return is_valid;
}

function validateBridgeIP(id, name, value) {
	var dhcp = document.getElementById("dhcp");
	if (!dhcp.checked) { return _validateIP(value) && value != "0.0.0.0"; }
	return true;
}

function validateBridgeNetmask(id, name, value) {
	var dhcp = document.getElementById("dhcp");
	if (!dhcp.checked) { return _validateNetmask(value) && value != "0.0.0.0"; }
	return true;
}

function validateBridgeOptIP(id, name, value) {
	var dhcp = document.getElementById("dhcp");
	if (!dhcp.checked && value.length > 0) { return _validateIP(value);  }
	return true;
}
var IPMODE_STATIC = 0;
var IPMODE_DHCP = 1;
var IPMODE_PPPOE = 2;
function chooseIPMode(mode){
	switch (mode) {
	case IPMODE_STATIC:
		$('.i_dhcp, .i_pppoe').disable();
		$('.i_static').enable();
		$('.dhcp, .pppoe').hide();
		$('.static, .mtu').show();
		break;
	case IPMODE_DHCP:
		$('.i_static, .i_pppoe').disable();
		$('.i_dhcp').enable();
		$('.static, .pppoe').hide();
		$('.dhcp, .mtu').show();
		break;
	case IPMODE_PPPOE:
		$('.i_static, .i_dhcp').disable();
		$('.i_pppoe').enable();
		$('.dhcp, .static, .mtu').hide();
		$('.pppoe').show();
		break;
	}
}
function validateWanIP(id, name, value) {
var val = getRadioValue("wlanipmode");
if (parseInt(val) == IPMODE_STATIC) { return _validateIP(value) && value != "0.0.0.0"; }
return true;
}
function validateWanNetmask(id, name, value) {
var val = getRadioValue("wlanipmode");
if (parseInt(val) == IPMODE_STATIC) { return _validateNetmask(value) && value != "0.0.0.0"; }
return true;
}
function validateWlanOptIP(id, name, value) {
var val = getRadioValue("wlanipmode");
if (parseInt(val) == IPMODE_STATIC && value.length > 0) { return _validateIP(value); }
return true;
}
function validateWlanPPPoE(id, name, value) {
var val = getRadioValue("wlanipmode");
if (parseInt(val) == IPMODE_PPPOE) { return value != null && value.length > 0; }
return true;
}
function validateLanIP(id,name,value) {
return _validateIP(value) && value != "0.0.0.0";
}
function validateLanNetmask(id,name,value) {
return _validateNetmask(value) && value != "0.0.0.0";
}
function validateDMZIP(id,name,value) {
var dmz = document.getElementById("dmz_status");
if (dmz.checked) { return _validateIP(value) && value != "0.0.0.0"; }
return true;
}
function dmzStatusClicked() {
var dmz = document.getElementById("dmz_status");
$('.i_dmz').disable(!dmz.checked);
$('.dmz').toggle(dmz.checked);
}
function natStatusClicked() {
var nat = $('#nat_status');
$('.nat').toggle(nat.is(':checked'));
}
function hwaddrStatusClicked() {
	var checked = $('#hwaddr_status').is(':checked');
	if (checked && ($hwaddr = $('#hwaddr')).val().length == 0) {
		$hwaddr.val($hwaddr.data('defmac'));
	}
	$('.i_hwaddr').disable(!checked);
	$('.hwaddr').toggle(checked);
}
function validateDhcpdIP(id,name,value) {
var c=document.getElementById('dhcpd_status');
if (c.checked) { return _validateIP(value) && value != "0.0.0.0"; }
return true;
}
function validateDhcpdNetmask(id,name,value) {
var c=document.getElementById('dhcpd_status');
if (c.checked) { return _validateNetmask(value) && value != "0.0.0.0"; }
return true;
}
function validateDhcpdDns(id, name, value) {
var c=document.getElementById('dhcpd_status');
var p=document.getElementById('dhcpd_dnsproxy_status');
if (c.checked && !p.checked && value.length > 0) { return _validateIP(value); }
return true;
}
function validateFallbackIP(id,name,value) {
var dhcp = document.getElementById("dhcp");
if (dhcp.checked) { return _validateIP(value) && value != "0.0.0.0"; }
return true;
}
function validateFallbackNetMask(id,name,value) {
var dhcp = document.getElementById("dhcp");
if (dhcp.checked) { return _validateNetmask(value) && value != "0.0.0.0"; }
return true;
}
function validatePppoeFallbackIP(id,name,value) {
var pppoe = document.getElementById("pppoe");
if (pppoe.checked && value.length > 0) { return _validateIP(value) && value != "0.0.0.0"; }
return true;
}
function validatePppoeFallbackNetMask(id,name,value) {
var pppoe = document.getElementById("pppoe");
if (pppoe.checked && value.length > 0) { return _validateNetmask(value) && value != "0.0.0.0"; }
return true;
}
function validateHwAddr(id,name,value) {
	var enabled = $('#hwaddr_status').is(':checked');
	if (enabled) {
		return (value != null && value.length > 0 &&
			/^([0-9A-F]{2}:?){5}[0-9A-F]{2}$/i.test(value) &&
			! /^(F{2}:?){5}F{2}$/i.test(value));
	}
	return true;
}
var DHCP_DISABLED = 0;
var DHCP_ENABLED = 1;
var DHCP_RELAY = 2;
function dhcpdDnsproxyClicked() {
var mode = getRadioValue("dhcpserver");
var p=document.getElementById('dhcpd_dnsproxy_status');
var enabled=(mode==DHCP_ENABLED) && !p.checked;
$('.i_dhcpd_dns').disable(!enabled);
$('.dhcpd_dns').toggle(enabled);
}
function chooseDhcpServer(mode){
	$('.i_dhcpd').disable(mode!=DHCP_ENABLED);
	$('.dhcpd').toggle(mode==DHCP_ENABLED);
	$('.i_dhcp_relay').disable(mode!=DHCP_RELAY);
	$('.dhcp_relay').toggle(mode==DHCP_RELAY);
	dhcpdDnsproxyClicked();
}

function _validateRouteTarget(ip, netmask) {
	var intIp = ipToInt(ip);
	var bitmask = ipToInt(netmask);
	return ((intIp & bitmask) == (intIp & intIp));
}

function ipToInt(ip) {
	var quad = ip.split('.');
	var intIp = 16777216 * parseInt(quad[0]) + 65536 * parseInt(quad[1]) +
                256 * parseInt(quad[2]) + parseInt(quad[3]);
	return intIp;
}
