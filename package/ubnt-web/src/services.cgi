#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
include("lib/system.inc");
include("lib/link.inc");
if ($cfg == -1) {
	include("lib/busy.tmpl");
	exit;
}

$ntp_status = cfg_get_def($cfg, "ntpclient.status", "disabled");
$ntp_server = cfg_get_def($cfg, "ntpclient.1.server", "0.ubnt.pool.ntp.org");

$pwdog_status = cfg_get_def($cfg, "pwdog.status", $pwdog_status);
$pwdog_host = cfg_get_def($cfg, "pwdog.host", $pwdogHost);
$pwdog_retry = cfg_get_def($cfg, "pwdog.retry", $pwdogRetry);
$pwdog_delay = cfg_get_def($cfg, "pwdog.delay", $pwdogDelay);
$pwdog_period = cfg_get_def($cfg, "pwdog.period", $pwdogPeriod);

$snmp_status = cfg_get_def($cfg, "snmp.status", "disabled");
$snmp_community = cfg_get_def($cfg, "snmp.community", "public");
$snmp_contact = cfg_get_def($cfg, "snmp.contact", $snmpContact);
$snmp_location = cfg_get_def($cfg, "snmp.location", $snmpLocation);

$dyndns_status = cfg_get_def($cfg, "dyndns.status", "disabled");
$dyndns_hostname = cfg_get_def($cfg, "dyndns.1.hostname", $dyndnshostname);
$dyndns_username = cfg_get_def($cfg, "dyndns.1.username", $dyndnsusername);
$dyndns_password = cfg_get_def($cfg, "dyndns.1.password", $dyndnspassword);

$message = "";
if ($REQUEST_METHOD=="POST") {
	if ($action == "httpscertupload" && strlen($httpscertdelete) > 0) {
		@unlink("/etc/persistent/https/server.crt");
		@unlink("/etc/persistent/https/server.key");
		cfg_save($cfg, $cfg_file);
		cfg_set_modified($cfg_file);
	} elseif ($action == "httpscertupload") {
		if (strlen($https_cert_file) == 0 && strlen($https_cert_key_file) == 0)
		{
			$error_msg = dict_translate("msg_no_https_cert_files_specified|No HTTPS certificate files specified.");
			include("lib/system.tmpl");
			exit;
		}
		$cert_error = check_uploaded_file($https_cert_file,
				$https_cert_file_size, dict_translate("HTTPS certificate"), 5120);
		$cert_key_error = check_uploaded_file($https_cert_key_file,
				$https_cert_key_file_size, dict_translate("HTTPS certificate key"), 5120);
		if (strlen($cert_error) > 0 || strlen($cert_key_error) > 0)
		{
			if (strlen($cert_error) > 0)
			{
				$error_msg = $cert_error;
			}
			else
			{
				$error_msg = $cert_key_error;
			}
			@unlink($https_cert_file);
			@unlink($https_cert_key_file);
			include("lib/system.tmpl");
			exit;
		}
		@mkdir("/etc/persistent/https/", 0755);
		if (strlen($https_cert_file))
		{
			exec("mv "+$https_cert_file+" /etc/persistent/https/server.crt");
		}
		if (strlen($https_cert_key_file))
		{
			exec("mv "+$https_cert_key_file+" /etc/persistent/https/server.key");
		}
		cfg_save($cfg, $cfg_file);
		cfg_set_modified($cfg_file);
	} else {
		if ($pwdogStatus != "enabled") {
			$pwdogStatus = "disabled";
		} else {
			cfg_set($cfg, "pwdog.host", $pwdogHost);
			cfg_set($cfg, "pwdog.retry", $pwdogRetry);
			cfg_set($cfg, "pwdog.delay", $pwdogDelay);
			cfg_set($cfg, "pwdog.period", $pwdogPeriod);
                        if ($pwdog_supp=="enabled") {
	                        cfg_set($cfg, "pwdog.command", "/bin/support /tmp/emerg /etc/persistent/emerg.supp emerg; reboot -f");
                        } else {
                        	cfg_del($cfg, "pwdog.command");
                        }
                }
		$pwdog_status = $pwdogStatus;
		$pwdog_host = $pwdogHost;
		$pwdog_retry = $pwdogRetry;
		$pwdog_delay = $pwdogDelay;
		$pwdog_period = $pwdogPeriod;
       		cfg_set($cfg, "pwdog.status", $pwdogStatus);
		if ($snmpStatus != "enabled") {
			$snmpStatus = "disabled";
		} else {
			cfg_set($cfg, "snmp.community", $snmpCommunity);
			cfg_set($cfg, "snmp.contact", $snmpContact);
			cfg_set($cfg, "snmp.location", $snmpLocation);
                }
                cfg_set($cfg, "snmp.status", $snmpStatus);
		$snmp_status = $snmpStatus;
		$snmp_community = $snmpCommunity;
		$snmp_contact = $snmpContact;
		$snmp_location = $snmpLocation;
		if ($ntpStatus != "enabled") {
			$ntpStatus = "disabled";
		} else {
			cfg_set($cfg, "ntpclient.1.status", $ntpStatus);
			cfg_set($cfg, "ntpclient.1.server", $ntpServer);
                }
       		cfg_set($cfg, "ntpclient.status", $ntpStatus);
		$ntp_status = $ntpStatus;
		$ntp_server = $ntpServer;
	       	cfg_set($cfg, "httpd.port", $httpport);
		if ($https_status != "enabled") {
			$https_status = "disabled";
		} elseif ($https_status == "enabled") {
       		       	cfg_set($cfg, "httpd.https.port", $httpsport);
                }
	        cfg_set($cfg, "httpd.https.status", $https_status);
		cfg_set($cfg, "httpd.session.timeout", IntVal($http_session_timeout) * 60);
		if ($telnetd_status != "enabled") {
			$telnetd_status = "disabled";
		}
       		cfg_set($cfg, "telnetd.status", $telnetd_status);
       	        if ($telnetd_status == "enabled") {
       			cfg_set($cfg, "telnetd.port", $telnetport);
		}
		if ($ssh_status != "enabled") {
		       	$ssh_status = "disabled";
		}
		if ($ssh_passwdauth != "enabled") {
			$ssh_passwdauth = "disabled";
		}
		cfg_set($cfg, "sshd.status", $ssh_status);
        	if ($ssh_status == "enabled") {
		      	cfg_set($cfg, "sshd.port", $sshport);
			cfg_set($cfg, "sshd.auth.passwd", $ssh_passwdauth);
	        }
		if ($syslog_status != "enabled") {
			$syslog_status = "disabled";
		}
		if ($rsyslog_status != "enabled") {
			$esyslog_status = "disabled";
		}
		cfg_set($cfg, "syslog.status", $syslog_status);
		cfg_set($cfg, "syslog.remote.status", $rsyslog_status);
                if ($syslog_status == "enabled" && $rsyslog_status == "enabled") {
			cfg_set($cfg, "syslog.remote.ip", $syslogip);
			cfg_set($cfg, "syslog.remote.port", $syslogport);
                }
		if ($dyndnsstatus != "enabled") {
			$dyndnsstatus = "disabled";
		} else {
			cfg_set($cfg, "dyndns.1.hostname", $dyndnshostname);
			cfg_set($cfg, "dyndns.1.username", $dyndnsusername);
			cfg_set($cfg, "dyndns.1.password", $dyndnspassword);
		}
		cfg_set($cfg, "dyndns.status", $dyndnsstatus);
		$dyndns_status = $dyndnsstatus;
		$dyndns_hostname = $dyndnshostname;
		$dyndns_username = $dyndnsusername;
		$dyndns_password = $dyndnspassword;
		if (strlen($discovery_status)) {
			$discovery_status = "enabled";
		}
		else {
			$discovery_status = "disabled";
		}
		cfg_set($cfg, "discovery.status", $discovery_status);
		if (strlen($cdp_status)) {
			$cdp_status = "enabled";
		}
		else {
			$cdp_status = "disabled";
		}
		cfg_set($cfg, "discovery.cdp.status", $cdp_status);

                cfg_update_dmz_mgmt($cfg);
		cfg_save($cfg, $cfg_file);
		cfg_set_modified($cfg_file);
        }
}

$https_status = cfg_get_def($cfg, "httpd.https.status", $https_status);
$telnetd_status = cfg_get_def($cfg, "telnetd.status", $telnetd_status);
$ssh_status = cfg_get_def($cfg, "sshd.status", $ssh_status);
$syslog_status = cfg_get_def($cfg, "syslog.status", $syslog_status);
$rsyslog_status = cfg_get_def($cfg, "syslog.remote.status", $rsyslog_status);
$httpport = cfg_get_def($cfg, "httpd.port", "80");
if (strlen($httpport) == 0) {
	$httpport = "80";
}
$httpsport = cfg_get_def($cfg, "httpd.https.port", "443");
if (strlen($httpsport) == 0) {
	$httpsport = "443";
}
$http_session_timeout = cfg_get_def($cfg, "httpd.session.timeout", "900");
if (strlen($http_session_timeout) == 0) {
	$http_session_timeout = "15";
} else {
	$http_session_timeout = IntVal($http_session_timeout) / 60;
}
$telnetport = cfg_get_def($cfg, "telnetd.port", "23");
if (strlen($telnetport) == 0) {
	$telnetport = "23";
}
$sshport = cfg_get_def($cfg, "sshd.port", "22");
if (strlen($sshport) == 0) {
	$sshport = "22";
}
$ssh_passwdauth = cfg_get_def($cfg, "sshd.auth.passwd", $ssh_passwdauth);
if (strlen($ssh_passwdauth) == 0) {
	$ssh_passwdauth = "enabled";
}
$syslogport = cfg_get_def($cfg, "syslog.remote.port", "514");
$syslogip = cfg_get_def($cfg, "syslog.remote.ip", $syslogip);
if (strlen($syslogport) == 0) {
	$syslogport = "514";
}

if (strlen($pwdog_retry) == 0) {
	$pwdog_retry = "3";
}
if (strlen($pwdog_period) == 0) {
	$pwdog_period = "300";
}
if (strlen($pwdog_delay) == 0) {
	$pwdog_delay = "300";
}

if (strlen(cfg_get_def($cfg, "pwdog.command", "")) > 0) {
	$pwdog_supp = 1;
} else {
	$pwdog_supp = 0;
}

$snmp_community = htmlspecialchars($snmp_community);
$snmp_contact = htmlspecialchars($snmp_contact);
$snmp_location = htmlspecialchars($snmp_location);

$discovery_status = cfg_get_def($cfg, "discovery.status", $discovery_status);
if (strlen($discovery_status) == 0) {
	$discovery_status = "enabled";
}
$mtik = cfg_get_def($cfg, "wireless.1.addmtikie", "disabled");
$cdp_status = cfg_get_def($cfg, "discovery.cdp.status", $cdp_status);
if (strlen($cdp_status) == 0) {
	$cdp_status = $mtik;
}

include("lib/services.tmpl");
>
