#!/sbin/cgi
<?
header("Content-Type: application/json");
if (isset($start) && $start == 1) {
	$command = "/usr/bin/spectralrecorder output_pathname=/tmp/airview.uavr";
	if (isset($selected_chain_mask_indices_bitmask)) {
		$command = "$command enabled_chain_mask_indices_bitset=$selected_chain_mask_indices_bitmask";
	} else {
		$command = "$command enabled_chain_mask_indices_bitset=1";
	}	
	if (isset($begin_frequency_mhz)) {
		$command = "$command begin_frequency_mhz=$begin_frequency_mhz";
	} else {
		$command = "$command begin_frequency_mhz=0";
	}
	if (isset($end_frequency_mhz) && $end_frequency_mhz > 0) {
		$command = "$command end_frequency_mhz=$end_frequency_mhz";
	} else {
		$command = "$command end_frequency_mhz=99999";
	}
	if (isset($max_duration_s) && $max_duration_s > 0) {
		$command = "$command max_duration_s=$max_duration_s";
	}
	if (isset($max_samples) && $max_samples > 0) {
		$command = "$command max_samples=$max_samples";
	}
	if (isset($maximum_size_kb)) {
		$command = "$command max_file_size_kb=$maximum_size_kb";
	} else {
		$command = "$command max_file_size_kb=0";
	}
	$command = "$command connect_with_server=1";
	exec("echo '#!/bin/sh'                         >/tmp/recorder_cmd.sh",$lines,$retval);
	exec("echo 'killall -9 spectralrecorder'      >>/tmp/recorder_cmd.sh",$lines,$retval);
	exec("echo 'rm -f /tmp/airview.uavr'           >>/tmp/recorder_cmd.sh",$lines,$retval);
	exec("echo 'airview start'                    >>/tmp/recorder_cmd.sh",$lines,$retval);
	exec("echo 'sleep 2'                          >>/tmp/recorder_cmd.sh",$lines,$retval);
	exec("echo '$command >/dev/console'           >>/tmp/recorder_cmd.sh",$lines,$retval);
	exec("chmod a+x /tmp/recorder_cmd.sh",$lines,$retval);
	bgexec(0,"/tmp/recorder_cmd.sh");
	sleep(1);
} 
if (isset($stop) && $stop == 1) {
	exec("echo 'killing recorder' >>/tmp/recorder_kill",$lines,$retval);
	exec("killall -9 spectralrecorder",$lines,$retval);
	sleep(1);
}
exec("ps | grep -v grep | grep spectralrecorder",$lines,$retval);
if($retval == 0) {
?>{ "airview": { "recording": 1 }}<?
} else {
?>{ "airview": { "recording": 0 }}<?
}
?>
