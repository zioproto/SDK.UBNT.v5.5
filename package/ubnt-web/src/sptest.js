var ticket = 0;
var max_time = 0;
var elapsed_time = 0;

function init()
{
	loadIPs();
	onAdvancedChanged();
}

var ip_regex = /^((25[0-5]|2[0-4]\d|[01]\d\d|\d?\d)\.){3}(25[0-5]|2[0-4]\d|[01]\d\d|\d?\d)$/

function validateInput()
{
	var ret = true;
	$('#results').html('');

	if ($('#dst_addr_select').val() == '0')
	{
		var ip_addr = $('#dst_addr_input').val();
		ret = ip_regex.exec(ip_addr) != null && ip_addr != '0.0.0.0';
		if (!ret)
		{
			$('#results').append('IP address is invalid.<br/>');
		}
	}

	if ($('#show_adv').is(':checked'))
	{
		var duration = parseInt($('#time_limit').val());
		if (duration == 0 || isNaN(duration))
		{
			ret = false;
			$('#results').append('Duration is invalid.<br/>');
		}
	}

	if ($('#auth_user').val().length == 0 || $('#auth_password').val().length == 0)
	{
		ret = false;
		$('#results').append('User/password can not be empty.<br/>');
	}

	return ret;
}

function getRemoteIp()
{
	return $('#dst_addr_select').val() == '0' ?
		$('#dst_addr_input').val() :
		$('#dst_addr_select option:selected').text();
}

function getResultsRequest(timeout)
{
	setTimeout(function() {
		var data = {};
		data["ticket"] = ticket;
		data["action"] = "status";

		$.ajax({
			url: 'sptest_action.cgi',
			cache: false,
			data: data,
			dataType: "json",
			success: handleStatus,
			error: handleError
		});
	}, timeout);
}

function sendStopRequest()
{
	var data = {};
	data["ticket"] = ticket;
	data["action"] = "stop";

	$.ajax({
		url: 'sptest_action.cgi',
		cache: false,
		data: data,
		dataType: "json",
		success: function() {}
	});
}

function handleStatus(response, textStatus)
{
	if (response.state == 10)
	{
		if (response.flags == 0)
		{
			var tx = toFixed(response.tx, 2);
			var rx = toFixed(response.rx, 2);
			$('#tx_results').html(tx + ' Mbps');
			$('#rx_results').html(rx + ' Mbps');
			var total = toFixed(parseFloat(tx) + parseFloat(rx), 2);
			$('#total_results').html('' + total + ' Mbps');
			$('#results').html('');
		}
		else
		{
			$('#results').html('Error: Speedtest timed out.');
		}

		enableForm(true);
		$('#loader').hide();
		sendStopRequest();
	}
	else
	{
		elapsed_time++;
		if (elapsed_time < max_time) {
			getResultsRequest(1000);
		}
		else {
			$('#results').html('Error: Speedtest timed out.');
			enableForm(true);
			$('#loader').hide();
			sendStopRequest();
		}
	}
}

function handleStart(response, textStatus)
{
	if (response.status != 0)
	{
		$('#results').html('Error: ' + response.message);
		enableForm(true);
		$('#loader').hide();
		return;
	}

	$('#results').html('');
	$('#loader').show();

	max_time = parseInt($('#time_limit').val()) + 5;
	elapsed_time = 0;

	getResultsRequest(1000);
}

function handleRemote(response, textStatus)
{
	if (response == null)
		window.location.reload();

	if (response.status != 0)
	{
		$('#results').html('Error: ' + response.message);
		enableForm(true);
		return;
	}

	var data = {};
	data["ticket"] = ticket;
	data["action"] = "start";
	data["target"] = getRemoteIp();
	data["port"] = $('#launcher_port').val();
	data["login"] = $('#auth_user').val();
	data["passwd"] = $('#auth_password').val();

	if ($('#show_adv').is(':checked'))
	{
		data["duration"] = $('#time_limit').val();
		data["direction"] = $('#direction_select').val();
	}

	$.ajax({
		cache: false,
		url: 'sptest_action.cgi',
		data: data,
		dataType: "json",
		success: handleStart,
		error: handleError
	});
}

function startTest()
{
	if (!validateInput())
		return;

	var generateId = function() {
		var id = '';
		for (var i = 0; i < 8; ++i)
			id += (((1+Math.random())*0x10000)|0).toString(16).substring(1);
		return id;
	};

	ticket = Math.floor(Math.random() * 1000);

	$('#results').html('Starting...');
	$('#tx_results').html('N/A');
	$('#rx_results').html('N/A');
	$('#total_results').html('N/A');
	enableForm(false);
	
	var data = {};
	data["ticket"] = ticket;
	data["action"] = "remote";
	data["target"] = getRemoteIp();
	data["port"] = $('#launcher_port').val();
	data["login"] = $('#auth_user').val();
	data["passwd"] = $('#auth_password').val();
	data["airosid"] = generateId();

	$.ajax({
		cache: false,
		url: 'sptest_action.cgi',
		data: data,
		dataType: "json",
		success: handleRemote,
		error: handleError
	});
}

function handleError(xhr, textStatus, errorThrown)
{
	enableForm(true);
	$('#loader').hide();
	if (xhr && xhr.status != 200 && xhr.status != 0)
		window.location.reload();
	else
		$('#results').html('Error: ' + textStatus);
}

function enableForm(enable)
{
	$('#runtest').enable(enable);
}

function refreshIPs(responseText)
{
	var options = '<option value="0">specify manually</option>';

	var ips = responseText.split('\n');
	for (i=0; i<ips.length; ++i)
	{
		if (ips[i].length > 0)
		{
			options += '<option value="' + i+1 + '">';
			options += ips[i] + '</option>';
		}
	}

	$('#dst_addr_select').html(options);
	onIpChanged();
}

$.fn.set_visible = function(visible) {
	if (visible)
		return this.show();
	else
		return this.hide();
}

$.fn.enable = function(enable) {
	if (enable)
		return this.removeClass('disabled').attr('disabled', false);
	else
		return this.attr('disabled', true).addClass('disabled');
}

function onIpChanged()
{
	$('#dst_addr_input').enable($('#dst_addr_select').val() == 0);
}

function onAdvancedChanged()
{
	var show_adv = $('#show_adv').is(':checked');
	$('#advanced1').set_visible(show_adv);
}

function loadIPs()
{
	$.post('ipscan.cgi?a='+new Date().getTime(), refreshIPs);
}

$(document).ready(function() {
	$('#runtest').click(function() { startTest(); return false; });
	$('#ip_refresh').click(function() { loadIPs(); return false; });
	$("#dst_addr_select").change(onIpChanged);
	$('#show_adv').change(onAdvancedChanged);
	init();
});
