#!/sbin/cgi
<?
if (isset($AIROS_SESSIONID) && strlen($AIROS_SESSIONID) > 0) {
	Exec("/bin/ma-deauth /tmp/.sessions.tdb " + $AIROS_SESSIONID);
	Header("Location: /login.cgi");
}
>
