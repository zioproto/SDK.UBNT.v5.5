#!/sbin/cgi
<?
include("lib/settings.inc");
if (cfg_is_modified($cfg_file) || (fileinode($fwup_lock) != -1)) {
	echo 300;
} else {
	echo 0;
}
>
