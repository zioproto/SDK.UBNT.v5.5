#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
include("lib/system.inc");

$max_keys = 10;
$max_file_len = 8192;

$ssh_key_regex = "^(ssh-(rsa|dss))[[:space:]]+([[:graph:]]+)[[[:blank:]]+(.*)]?$";

$close_window = 0;

Function sshd_save_cfg
(
	global $cfg, $cfg_file, $notify_parent;
	cfg_save($cfg, $cfg_file);
	cfg_set_modified($cfg_file);
	$notify_parent = 1;
);

Function sshd_find_unused
(
	global $cfg, $max_keys;
	$idx = 1; $found = 0;
	while ($idx <= $max_keys && $found == 0) {
		$status = cfg_get($cfg, "sshd.auth.key.$idx.status");
		if (strlen($status) == 0) {
			$found = $idx;
		}
		$idx++;
	}

	if ($found == 0) {
		$found = $idx;
	}

	return $found;
);

Function sshd_add_key $idx, $type, $value, $comment
(
	global $cfg;
	cfg_set($cfg, "sshd.auth.key.$idx.status", "enabled");
	cfg_set($cfg, "sshd.auth.key.$idx.type", $type);
	cfg_set($cfg, "sshd.auth.key.$idx.value", $value);
	cfg_set($cfg, "sshd.auth.key.$idx.comment", $comment);
);

Function sshd_remove_key $idx
(
	global $cfg, $max_keys;
	cfg_del($cfg, "sshd.auth.key.$idx.status");
	cfg_del($cfg, "sshd.auth.key.$idx.type");
	cfg_del($cfg, "sshd.auth.key.$idx.value");
	cfg_del($cfg, "sshd.auth.key.$idx.comment");

	# shift all subsequent sshd keys "up"
	$i = $idx + 1; $used = 1;
	while ($i <= $max_keys && $used == 1) {
		$status = cfg_get($cfg, "sshd.auth.key.$i.status");
		if (strlen($status) > 0) {
			$j = $i - 1;
			cfg_rename($cfg, "sshd.auth.key.$i.status", "sshd.auth.key.$j.status");
			cfg_rename($cfg, "sshd.auth.key.$i.type", "sshd.auth.key.$j.type");
			cfg_rename($cfg, "sshd.auth.key.$i.value", "sshd.auth.key.$j.value");
			cfg_rename($cfg, "sshd.auth.key.$i.comment", "sshd.auth.key.$j.comment");
			$i++;
		}
		else {
			$used = 0;
		}
	}
);

if ($REQUEST_METHOD=="POST")
{
	if ($action == "save") {
		$close_window = 1;
		$save_config = 0; $i = 1;
		while ($i <= $max_keys) {
			$status = "status_$i";
			$comment = "comment_$i";
			
			if (isset($$status)) {
				cfg_set($cfg, "sshd.auth.key.$i.status", $$status);
				$save_config = 1;
			}
			
			if (isset($$comment)) {
				cfg_set($cfg, "sshd.auth.key.$i.comment", $$comment);
				$save_config = 1;
			}

			$i++;
		}
		if ($save_config == 1) {
			sshd_save_cfg();
		}
	}

	if (strlen($keyfile) > 0) {
		$close_window = 0;
		$keyfile_error = check_uploaded_file(
			$keyfile, $keyfile_size, dict_translate("public key"), $max_file_len);
		if (strlen($keyfile_error) > 0) {
			$error_msg = $keyfile_error;
		}
		else {
			$idx = sshd_find_unused();
			$last_idx = $idx;

			$i = 0;
			$arr = File($keyfile);
			while ($i < count($arr) && strlen($error_msg) < 1) {
				if (ereg($ssh_key_regex, $arr[$i], $regs)) {
					if ($idx <= $max_keys) {
						sshd_add_key($idx, $regs[1], $regs[3], $regs[4]);
						$idx++;
					}
					else {
						$error_msg = dict_translate("Maximum number of public keys reached.");
					}
				}
				else {
					$error_msg += Sprintf(dict_translate("Invalid entry at line %d."), $i + 1);
				}
				$i++;
			}

			if ($last_idx != $idx) {
				sshd_save_cfg();
			}
		}

		@unlink($keyfile);
	}
	elseif ($rm_idx > 0) {
		$close_window = 0;
		sshd_remove_key(IntVal($rm_idx));
		sshd_save_cfg();
	}
}

include("lib/sshd_authkeys_head.tmpl");

$i = 1; $used = 1;
while ($i <= $max_keys && $used == 1) {
	$status = cfg_get($cfg, "sshd.auth.key.$i.status");
	if (strlen($status) > 0) {
		$type = cfg_get_def($cfg, "sshd.auth.key.$i.type", "ssh-rsa");
		$typename = "RSA";
		if ($type == "ssh-dss") {
			$typename = "DSA";
		}
		$key = cfg_get($cfg, "sshd.auth.key.$i.value");
		if (strlen($key) > 32) {
			$key_str = substr($key, 0, 16) + "&nbsp; ... &nbsp; " + substr($key, strlen($key) - 16, 16);
		}
		else {
			$key_str = $key;
		}
		$comment = cfg_get_def($cfg, "sshd.auth.key.$i.comment", "");

		include("lib/sshd_authkeys_item.tmpl");
	}
	else {
		$used = 0;
	}
	$i++;
}

include("lib/sshd_authkeys_tail.tmpl");
