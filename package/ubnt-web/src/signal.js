var RSSI_Max = 30;

var colors = ["#d2001b", "#e0201a", "#f2541b", "#f8901e", "#fac922", "#faeb26", "#eaed26", "#d3ed25", "#b9ed25", "#9bee25", "#7fe726", "#64dd26", "#4cd228", "#3ac82a", "#2ec02d", "#28ba2f"];

function update_meter(name, val, max_val) {
	var v = Math.floor(100 * val / max_val);
	if (v > 100) v = 100;
	$('#'+name).css({'width':''+v+'%'}).show();
}

function rssi_to_signal(rssi, noisefloor) {
	return 0 + noisefloor + rssi;
}

function log10(x) {
	// Log 10 calculation
	return(Math.log(x) / Math.log(10));
}

function rssi_combine(rssi, rssiext) {
	if (rssiext == 0)
		return rssi;

	var mw = Math.pow(10, rssi / 10);
	var mwext = Math.pow(10, rssiext / 10);
	var combi_rssi = log10(mw + mwext) * 10;

	return Math.round(combi_rssi);
}

function updateSignalLevel(signal, rssi, noisef, chwidth, rx_chainmask, chainrssi, chainrssiext) {
	$('#noisef').text(noisef != 0 ? noisef + ' dBm' : '-');

	if (!$('#signalinfo').is(':visible'))
		return;

	var active_chain_count = 0;
	var refnf = signal - rssi;

	if (chwidth != null) {
		var i;
		for (i = 0; i < 3; ++i) {
			if (rx_chainmask & (1 << i)) {
				$('#signal_'+i).text(rssi_to_signal(chainrssi[i], refnf));
				active_chain_count++;				
			}
		}
	}

	$('#signal_chain').toggle(active_chain_count > 1);
	update_meter('signalbar', rssi, RSSI_Max);
	$('#signal').text(signal != 0 ? signal + ' dBm' : '-');
}
