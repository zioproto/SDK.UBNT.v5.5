#!/sbin/cgi
<?
include("lib/settings.inc");
if (fileinode($cfg_file_bak) != -1) {
$cfg = @cfg_load($cfg_file_bak);
} else {
$cfg = @cfg_load($cfg_file);
}
include("lib/l10n.inc");
include("lib/throughput.tmpl");
>
