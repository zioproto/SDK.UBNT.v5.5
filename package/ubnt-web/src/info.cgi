#!/sbin/cgi
<?
  SecureVar("cmd*");
  SecureVar("lines");
  include("lib/settings.inc");
  $cfg = @cfg_load($cfg_file);
  include("lib/l10n.inc");

  $wmode = w_get_mode($wlan_iface);

  if ($REQUEST_METHOD == "POST") {
  } 

  /* TODO: retrieve common variables for Managed mode*/
  if ($wmode == 1) {
  } else {
  }
  include("lib/status.tmpl");
>
