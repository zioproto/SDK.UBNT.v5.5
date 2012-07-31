var IP_regex = /^((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\.){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])$/;
var HOSTNAME_regex = /^(([a-zA-Z0-9\-]*[a-zA-Z0-9])\.)*([A-Za-z0-9\-]*[A-Za-z0-9])$/; 

function validateIP(input) {
 return IP_regex.exec(input) != null;
}

function validateLogoURL(id, name, value) {
 var logoStatus = document.getElementById('logoStatus');
 if (logoStatus.checked) {
   return value.length > 0;
 }
 return true;
}
function validateLogoFile(id, name, value) {
 var logoStatus = document.getElementById('logoStatus');
 if (logoStatus.checked) {
   if (value.length == 0) {
     return true;
   }
   if (value.length < 4) {
     return false;
   }
   var ext = value.substring(value.length - 4);
   if (ext.toLowerCase() != ".gif") {
     return false;
   }
 }
 return true;
}
function logoStatusClicked() {
var c=document.getElementById('logoStatus');
statusClicked(c,new Array('logoURL','logoFile'));
var s=document.getElementById('logoSubmit');
s.value=!c.checked ? system_l10n_change : system_l10n_upload;
}
function pwdogStatusClicked() {
var c=document.getElementById('pwdogStatus');
statusClicked(c,new Array('pwdogHost','pwdogRetry','pwdogDelay','pwdogPeriod', 'pwdog_supp'));
}
function validatePwdog(frame,err) {
var c=document.getElementById('pwdogStatus');
if (c.checked)
	return validateStandard(frame,err);
return true;
}
function validatePwdogHost(id, name, value) {
var c=document.getElementById('pwdogStatus');
if (c.checked) { return validateIP(value) && value != "0.0.0.0"; }
return true;
}
function snmpStatusClicked() {
var c=document.getElementById('snmpStatus');
statusClicked(c,new Array('snmpCommunity','snmpContact','snmpLocation'));
}
function ntpStatusClicked() {
var c=document.getElementById('ntpStatus');
statusClicked(c,new Array('ntpServer'));
}
function validateSnmp(frame,err) {
var c=document.getElementById('snmpStatus');
if (c.checked)
	return validateStandard(frame,err);
return true;
}
function validateBTP(frame,err) {
var c=document.getElementById('ntpStatus');
if (c.checked)
	return validateStandard(frame,err);
return true;
}
function httpsStatusClicked() {
var c=document.getElementById('https_status');
statusClicked(c,new Array('httpsport'));
}
function telnetStatusClicked() {
var c=document.getElementById('telnetd_status');
statusClicked(c,new Array('telnetport'));
}
function sshStatusClicked() {
$('.i_sshd').disable(!$('#ssh_status').is(':checked'));
}
function syslogStatusClicked() {
var c=document.getElementById('syslog_status');
var r=document.getElementById('rsyslog_status');
if (r.checked) {
statusClicked(c,new Array('rsyslog_status', 'syslogip', 'syslogport'));
} else {
statusClicked(c,new Array('rsyslog_status'));
statusClicked(r,new Array('syslogip', 'syslogport'));
}
}
function rsyslogStatusClicked() {
var c=document.getElementById('rsyslog_status');
statusClicked(c,new Array('syslogip', 'syslogport'));
}
function validateSyslogHost(id, name, value) {
var c=document.getElementById('syslog_status');
var r=document.getElementById('rsyslog_status');
if (c.checked && r.checked) { return validateIP(value) && value != "0.0.0.0"; }
return true;
}
function validateRoUser() {
	var enabled = $('#ro_status').is(':checked');
	if (enabled) {
		var roname = $.trim($('#rousername').val());
		var adminname = $.trim($('#adminname').val());
		return (roname && (roname.length > 0) && (roname != adminname));
	}
	return true;
}
function roUserStatusClicked() {
	var enabled = $('#ro_status').is(':checked');
	$('.i_rouser').enable(enabled);
	$('.rouser').toggle(enabled);
	$('.ropasswd').hide();
	$('.i_ropasswd').disable();
}
function dateStatusChanged() {
	$('#systemdate_ui').datepicker(
			$('#date_status').is(':checked') ? 'enable' : 'disable');
}

function pad_date_num(num) {
	return num < 10 ? '0' + num : num.toString();
}

function appendSystemDate(curr_form) {
	var systemdate = $('#systemdate_ui').val();
	if (systemdate.length > 0) {
		var d = new Date(systemdate);
		systemdate = '';
		if (!isNaN(d.getTime())) {
			systemdate = systemdate.concat(
				pad_date_num(d.getMonth() + 1),
				pad_date_num(d.getDate()),
				'0000',
				d.getFullYear().toString());
		}
	}
	curr_form.append('<input type="hidden" name="systemdate" value="' + systemdate + '"/>');
}

function validateCoord(id, name, value) {
	if (value.length == 0)
		return true;
	var fval = parseFloat(value);
	return (!isNaN(fval) && fval <= 180 && fval >= -180);
}

function validateNTPServer(id, name, value) {
	if (validateIP(value))
		return (value != "0.0.0.0");
	return HOSTNAME_regex.test(value);
}

function dyndnsStatusClicked() {
var c=document.getElementById('dyndnsstatus');
statusClicked(c,new Array('dyndnshostname','dyndnsusername','dyndnspassword','_dyndnspassword_x'));
}
