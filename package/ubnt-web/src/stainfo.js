var si_refresh_timeout = 0;

function setHeading(si) {
	var heading;
	if (si_global.mode == 'ap') {
		heading = $.l10n._("Access Point") + ' &nbsp; &nbsp; &nbsp; &nbsp; ' + si.mac;
	}
	else {
		heading = $.l10n._("Station") + ' &nbsp; &nbsp; &nbsp; &nbsp; ' + si.mac;
		if (si.aprepeater != 0)
			heading += ' (' + $.l10n._('AP-Repeater') + ')';
		heading += ' &nbsp; &nbsp; [ ' + si.associd + ' ]';
	}
	$('#si_heading').html(heading);
}

function setAck(ack, distance) {
	var dist = (distance < 150) ? 150 : distance;
	var dist_km = toFixed(dist / 1000.00, 1);
	var dist_mi = toFixed(dist / 1609.344, 1);
	var ack_val = [dist_mi, 'miles', '(' + dist_km, 'km)'];
	$('#si_ack').text(ack_val.join(' '));
}

function decodeAirMaxPri(priority) {
	var label;
	switch (priority) {
		case 0:
			label = $.l10n._('High');
			break;
		case 1:
			label = $.l10n._('Medium');
			break;
		case 2:
			label = $.l10n._('Low');
			break;
		default:
			label = $.l10n._('None');
	}
	return label;
}

function formatRate(rate) {
	if (rate > 0)
		return '' + toFixed(rate, 1) + ' Mbps';
	else
		return ' - ';
}

function formatBytes(bytes) {
	var bytesStr = '' + bytes;
	if (bytes > 1024*1024*1024)
		bytesStr += ' (' + toFixed(bytes / (1024*1024*1024), 2) + ' GBytes)';
	else if (bytes > 1024*1024)
		bytesStr += ' (' + toFixed(bytes / (1024*1024), 2) + ' MBytes)';
	else if (bytes > 1024)
		bytesStr += ' (' + toFixed(bytes / 1024, 2) + ' kBytes)';

	return bytesStr;
}

function fillRates(si) {
	$('#si_rate_tbl > tbody').empty();

	var tbody = [];
	for (var i = 0; i < si.rates.length; ++i) {
		var row = [];
		row.push('<tr>');

		row.push('<td align="center">' + si.rates[i] + '</td>');
		var signal = (si.signals[i] != 0 && si.signals[i] != -96) ? si.signals[i] : $.l10n._('N/A');
		row.push('<td align="center">' + signal + '</td>');

		row.push('</tr>');
		tbody.push(row.join(''));
	}

	$('#si_rate_tbl > tbody').append(tbody.join(''));
}

function loadStaInfo(sinfo) {
	if (sinfo.length != 1) {
		$('#si_details').hide();
		$('#si_kick').hide();
		if (si_global.sta_mac != '00:00:00:00:00:00')
			$('#si_heading').text('[ ' + si_global.sta_mac + ' ] ' + $.l10n._('Not Associated'));
		else
			$('#si_heading').text($.l10n._('Not Associated'));
		return;
	}

	var si = sinfo[0];

	setHeading(si);
	$('#si_name').text(si.name.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;'));
	$('#si_uptime').text(secsToCountdown(si.uptime, $.l10n._('day'), $.l10n._('days')));
	$('#si_signal').text('' + si.signal + ' dBm');
	$('#si_noise').text('' + si.noisefloor + ' dBm');
	if (si_global.autoack)
		setAck(si.ack, si.distance);
	$('#si_ack_row').toggle(si_global.autoack);
	$('#si_ccq').text((si.ccq > 0) ? '' + si.ccq + '%' : '-' );

	if (si_global.airmax_on) {
		$('#si_amp').text(decodeAirMaxPri(si.airmax.priority));
		$('#si_amq').text('' + si.airmax.quality + '%');
		$('#si_amc').text('' + si.airmax.capacity + '%');
	}
	$('.si_airmax').toggle(si_global.airmax_on);
    
    if (si_global.airmax_on && si_global.phased_array) {
        $('#si_beam').html((si.airmax.beam < si_global.beam_angles.length ? si_global.beam_angles[si.airmax.beam] : si_global.beam_angles[si_global.beam_angles.length - 1]));
    }    
    $('.si_airbeam').toggle(si_global.airmax_on && si_global.phased_array);

	if (si.lastip != '0.0.0.0')
		$('#si_lastip').html('<a href="http://' + si.lastip + '" target="_blank">' + si.lastip + '</a>');
	else
		$('#si_lastip').text($.l10n._('unknown'));

	$('#si_rate').text(formatRate(si.tx) + ' / ' + formatRate(si.rx));
	$('#si_packets').text('' + si.stats.tx_data + ' / ' + si.stats.rx_data);
	$('#si_packet_rate').text('' + si.stats.tx_pps + ' / ' + si.stats.rx_pps);
	$('#si_txbytes').text(formatBytes(si.stats.tx_bytes));
	$('#si_rxbytes').text(formatBytes(si.stats.rx_bytes));

	fillRates(si);

	$('#si_details').show();
	if (si.aprepeater == 0) {
		$('#si_kick').disable(0).show();
	}
}

function handleError(xhr, textStatus, errorThrown) {
	if (xhr && xhr.status != 200 && xhr.status != 0)
		window.location.reload();
}

function refreshStaInfo() {
	var data = {};
	data['ifname'] = si_global.ifname;
	data['sta_mac'] = si_global.sta_mac;

	$.ajax({
		cache: false,
		url: '/sta.cgi',
		data: data,
		dataType: 'json',
		success: loadStaInfo,
		error: handleError,
		complete: function(xhr, status) {
			if (typeof refreshContent != 'function') {
				if (si_refresh_timeout)
					clearTimeout(si_refresh_timeout);
				si_refresh_timeout = setTimeout(refreshStaInfo, 5000);
			}
		}
	});
	return false;
}

function refreshAll() {
	if (typeof reloadStatus == 'function')
		reloadStatus();
	refreshStaInfo();
}

function kickEnd(data, textStatus, xhr) {
	refreshStaInfo();
}

function kickStation() {
	$('#si_kick').disable();
	jQuery.ajax({
		url: kick_url,
		cache: false,
		dataType: "json",
		success: kickEnd,
		error: handleError
		});
	return false;
}

$(document).ready(function() {
	$.l10n.init({ dictionary: l10n_stainfo });
	$('#si_details').hide();
	$('#si_kick').hide();
	if (!window.opener)
		$('#si_close').hide();
	refreshStaInfo();
});
