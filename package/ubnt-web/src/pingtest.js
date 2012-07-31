var ping_running = false;
var ping_count = 0;
var ping_form = null;
var post_data = {};

var min;
var max;
var avg;
var sent_packets;
var received_packets;

function updateStatus() {
	var loss = "0";
	if (received_packets < sent_packets)
		loss = Math.round((sent_packets - received_packets) * 100 / sent_packets);

	$('#status_rcv').text(received_packets);
	$('#status_sent').text(sent_packets);
	$('#status_loss').text(loss);

	if (received_packets > 0) {
		$('#status_min').text(min);
		$('#status_avg').text(Math.round(avg * 100) / 100);
		$('#status_max').text(max);
	}
}

function addRow(columns) {
	var row = [];
	row.push('<tr class="pingrow">');

	var i;
	for (i = 0; i < columns.length; ++i)
		row.push("<td class=\"str\">" + columns[i] + "</td>");

	row.push('</tr>');

	$('#pingdata > tbody').append(row.join(''));
	$('#scroll_results').scrollTop($('#scroll_results')[0].scrollHeight);
	updateStatus(); 
}

function addResult(ip_addr, response_time_ms, ttl) {
	received_packets++;
	if (response_time_ms > max)
		max = response_time_ms;
	if (response_time_ms < min)
		min = response_time_ms;
	avg += (response_time_ms - avg) / received_packets;
	addRow([ip_addr, "" + response_time_ms + " ms", ttl]);
}

function initPing() {
	var ip_addr = $('#dst_addr_select').val();
	if (ip_addr == '0')
		ip_addr = $('#dst_addr_input').val();
	if (ip_addr.length == 0)
		return false;

	post_data["ip_addr"] = ip_addr;
	post_data["ping_size"] = $('#ping_size').val();
	ping_count = $('#ping_count').val();

	min = 9999999.9;
	max = 0.0;
	avg = 0.0;
	sent_packets = 0;
	received_packets = 0;
	return true;	
}

function handleResponse(data, textStatus, xhr) {
	sent_packets++;
	if (textStatus == "success" && xhr.status == 200) {
		results = data.split('|');
		if (results.length > 0) {
			rc = parseInt(results[0]);
			if (rc == 0)
				for (i = 1; i < results.length; i += 3)
					addResult(results[i], parseFloat(results[i+1]), parseInt(results[i+2]));
			else if (rc == -4)
				addRow([post_data["ip_addr"], pingtest_l10n_timeout, '&nbsp;&nbsp;&nbsp;']);
		}
	}
}

function handleError(xhr, textStatus, errorThrown) {
	if (xhr && xhr.status != 200 && xhr.status != 0)
		window.location.reload();
	else
		stopPing();
}

function doPing() {
	$.ajax({
		cache: false,
		url: '/pingtest_action.cgi',
		data: post_data,
		success: handleResponse,
		error: handleError,
		complete: function(xhr, status) {
			if (ping_running) {
				if (ping_count > sent_packets)
					doPing();
				else
					stopPing();
			}
		}
	});
}

function startPing(form) {
	ping_form = form;
	if (ping_running || !initPing())
		return false;
	ping_running = true;
	pingStarted();
	doPing();
	return true;
}

function runPing(form) {
	if (!ping_running)
		startPing(form);
	else
		stopPing();
}

function stopPing() {
	ping_running = false;
	pingStopped();
}

function pingStarted() {
	$('#ping').val(l10n_stop);

	$('.pingrow').remove();
	$('#dst_addr_select, #dst_addr_input, #ping_count, #ping_size').disable();
	$('.status').text('0');
}

function pingStopped() {
	$('#ping').val(l10n_start);

	if (iplist) { iplist.triggerManual(ping_form.dst_addr_select); }
	$('#dst_addr_select, #dst_addr_input, #ping_count, #ping_size').enable();
}
