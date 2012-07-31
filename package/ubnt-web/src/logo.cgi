#!/sbin/cgi
<?  
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");

if ($cfg == -1) {
	  include("lib/busy.tmpl");
	  exit;
}
$logo_enabled = cfg_get_def($cfg, "ls_logo.status", "disabled");
if ($logo_enabled == "enabled") {
	$logo_url = cfg_get_def($cfg, "ls_logo.url", "http://www.ubnt.com");
	$logo_pic = "logo.gif";
}
include("lib/logo.tmpl");
>
