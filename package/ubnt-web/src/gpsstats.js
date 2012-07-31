function getSnr(snr) {
	var value = snr * 2;
	if (value < 10) value = 10
	else if (value > 100) value = 100;
	return value;
}

function refreshTable(data) {
	if (!data)
		return;

	$('#sats > tbody').empty();

	var tbody = [];
	for (var i = 0; i < data.length; ++i) {
		var entry = data[i];
		
		var row = [];
		row.push('<tr>');

		row.push('<td class="centered">' + ((entry.sat >= 10) ? entry.sat : '0' + entry.sat) + '</td>');
		row.push('<td style="line-height: 1;"><span class="percentborder"><div class="mainbar" style="width: ' + getSnr(entry.snr) + '%;">&nbsp;</div></span></td>');

		if (i + 1 < data.length) {
			entry = data[++i];
			row.push('<td class="centered">' + ((entry.sat >= 10) ? entry.sat : '0' + entry.sat) + '</td>');
			row.push('<td style="line-height: 1;"><span class="percentborder"><div class="mainbar" style="width: ' + getSnr(entry.snr) + '%;">&nbsp;</div></span></td>');
		}
		else {
			row.push('<td>&nbsp;</td><td>&nbsp;</td>');
		}

		row.push('</tr>');
		tbody.push(row.join(''));
	}

	if (data.length == 0 || typeof data.length == 'undefined')
		tbody.push('<tr><td colspan="' + $('#sats >thead >tr >th').length + '">' + $.l10n._('No data available') + '</td></tr>');

	$('#sats > tbody').append(tbody.join(''));
}

function refreshInfo() {
	var data = {};
	data["load"] = 'y';
	$.ajax({
		type: "GET",
		url: "gpsstats.cgi",
		dataType: "json",
		cache: false,
		data: data,
		success: refreshTable,
		complete: requestCompleted
	});
}

function requestCompleted(xhr, status) {
	if (typeof requestCompleted.timeout == 'undefined')
		requestCompleted.timeout = 0;

	if (requestCompleted.timeout)
		clearTimeout(requestCompleted.timeout);

	requestCompleted.timeout = setTimeout(refreshInfo, 1000);
}


function refreshAll() {
	if (typeof reloadStatus == 'function')
		reloadStatus();
	refreshInfo();
}

$(document).ready(function() {
	$.l10n.init({ dictionary: l10n_gpsstats });
	refreshInfo();
});
