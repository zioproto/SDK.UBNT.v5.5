#!/sbin/cgi
<?
include("lib/settings.inc");
include("lib/l10n.inc");

$update_info = "/tmp/.update-info.json";

$cmd_wget = "/usr/bin/wget";
$check_url = "http://www.ubnt.com/update/check.php";
$check_interval = 86400; # 24 hours

function need_to_check
(
	global $update_info, $check_interval;
	$do_check = 1;

	$last_checked = fileMtime($update_info);
	if ($last_checked != -1) {
		$diff = Time() - $last_checked;
		if ($diff < $check_interval) {
			$do_check = 0;
		}
	}
	return $do_check;
);

function check_for_update
(
	global $board_id, $check_url, $cmd_wget, $update_info;
	$version = @file("/usr/lib/version");
	$params = "sysid=$board_id&fwver=$version";
	exec("$cmd_wget -O $update_info '$check_url?$params' >/dev/null 2>&1");
);

if (IsSet($force) || need_to_check()) {
	check_for_update();
}

header("Content-Type: application/json");
if (fileInode($update_info) != -1) {
	PassThru("cat $update_info");
}
else {
	echo "{ \"update\": \"false\" }";
}

>
