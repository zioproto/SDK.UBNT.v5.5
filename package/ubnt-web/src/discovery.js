$(document).ready(function() {
	mainTable = $('#devices').dataTable({
		'aaSorting': [ [1,'asc'] ],
		'iDisplayLength' : 20,
		'bLengthChange': false,
		'bPaginate': true,
		'bSortClasses': false,
		'bAutoWidth': true,
		'sPaginationType': 'full_numbers',
		'oLanguage': {
			'sEmptyTable': 'Loading, please wait...',
			'oPaginate': {
				'sFirst':    '<<',
				'sPrevious': '<',
				'sNext':     '>',
				'sLast':     '>>'
			}
		},
		'fnRowCallback': function (nRow, aData, iDisplayIndex, iDisplayIndexFull) {
			if (aData[ip_col].length > 0) {
				var url = '<a href="http://' + aData[ip_col] + '" target="_blank">' + aData[ip_col] + '</a>';
				$('td:eq(' + ip_col + ')', nRow).html(url);
			}
			return nRow;
		},
		'fnInitComplete': function (oSettings) {
			oSettings.oLanguage.sEmptyTable = 'No devices found.';
		}
		});

	doDiscovery();
});


	

function doDiscovery() {
	var duration = getInterval()[1];

	var data = {};
	data["discover"] = 'y';
	data["duration"] = duration;
	$.ajax({
		type: "GET",
		url: "discovery.cgi",
		dataType: "json",
		cache: false,
		data: data,
		success: processJSON,
		error: processError,
		complete: requestCompleted,
		timeout: duration + 5000
	});
	curr_poll++;
}

function processJSON(json) {
	updateData(json);
	if ($('#loader').is(':visible')) {
		$('#loader').hide();
		$('#results').show();
		$('#btn_row').show();
	}
}

function processError(xhr, err, e) {
	if ($('#loader').is(':visible'))
		$('#loader').hide();
}

function requestCompleted(xhr, status) {
	if (typeof requestCompleted.timeout == 'undefined')
		requestCompleted.timeout = 0;

	if (requestCompleted.timeout)
		clearTimeout(requestCompleted.timeout);

	requestCompleted.timeout = setTimeout(doDiscovery, getInterval()[0]);
}

function getDeviceInfo(device) {
	var result = [device.hwaddr, device.hostname, decodeWmode(device.wmode),
		device.essid, device.product, decodeFwVersion(device.fwversion), device.ipv4];
	$.each(result, function(i, val) {
		if (val.length == 0)
			result[i] = "&nbsp;";
	});
	
	return result;
}

function addDevice(hwaddr, device, last_seen) {
	var idx = mainTable.fnAddData(getDeviceInfo(device), true);
	mainTable.fnSettings().aoData[idx[0]].nTr.id = hwaddr;
	devices[hwaddr] = { 'last_seen': last_seen, 'device': device };
}

function removeDevice(hwaddr, device) {
	var nodes = mainTable.fnGetNodes();
	var pos = $(nodes).filter('#' + hwaddr)[0];
	if (pos) {
		mainTable.fnDeleteRow(pos, null, false);
		delete device;
	}
	return (pos != null);
}

function updateData(data) {
	var now = new Date();
	var curr = mainTable.fnSettings()._iDisplayStart;

	for (var i = 0; data && i < data.devices.length; i++) {
		var device = data.devices[i];
		var hwaddr = device.hwaddr.replace(/:/g, '');
		if (!devices[hwaddr]) {
			addDevice(hwaddr, device, now);
		}
		else {
			if (devEquals(device, devices[hwaddr].device)) {
				devices[hwaddr].last_seen = now;
			}
			else {
				/* faster than update */
				if (removeDevice(hwaddr, devices[hwaddr]))
					addDevice(hwaddr, device, now);
			}
		}
	}

	for (var hwaddr in devices) {
		var diff = now - devices[hwaddr].last_seen;
		if (diff > expire_after)
			removeDevice(hwaddr, devices[hwaddr]);
	}

	mainTable.fnSettings()._iDisplayStart = curr;
	mainTable.fnDisplayStart(mainTable.fnSettings());
}

function getInterval() {
	return (curr_poll < intervals.length) ? intervals[curr_poll] : intervals[intervals.length - 1];
}

function decodeWmode(wmode) {
	return (wmode != 3) ? 'STA' : 'AP';
}

function decodeFwVersion(fwversion) {
	var rg = /^\w+\.\w+\.(.+)\.\d+\.\d+\.\d+$/;
	rg_res = rg.exec(fwversion);
	if (rg_res != null)
		return rg_res[1];
	return '&nbsp;';
}

function devEquals(dev1, dev2) {
	var props = ['hostname', 'product', 'wmode'];
	for (prop in props) {
		if (dev1[props[prop]] != dev1[props[prop]])
			return false;
	}
	return equals(dev1.addresses, dev2.addresses);
}

function hasProps(obj1, obj2) {
	for (p in obj1)
		if (typeof(obj2[p]) == 'undefined')
			return false;
	return true;
}

function equals(x, y) {
	if (!hasProps(x, y) || !hasProps(y, x))
		return false;
	
	for (p in x) {
		switch (typeof(x[p]))
		{
			case 'object':
				if (!equals(x[p], y[p]))
					return false;
				break;
			case 'function':
				if (x[p].toString() != y[p].toString())
					return false;
				break;
			default:
				if (x[p] != y[p])
					return false;
		}
	}

	return true;
}

$.fn.dataTableExt.oApi.fnDisplayStart = function (oSettings) {
	oSettings.oApi._fnCalculateEnd(oSettings);
	oSettings.oApi._fnDraw(oSettings);
}

$.fn.dataTableExt.aTypes.push(
	function (data) {
		if (/^\d{1,3}[\.]\d{1,3}[\.]\d{1,3}[\.]\d{1,3}$/.test(data))
			return 'ip-address';
		return null;
	}
);

$.fn.dataTableExt.oApi.fnSortByIp = function(ip1, ip2, asc) {
	var l = ip1.split('.');
	var r = ip2.split('.');
	var ret = asc ? -1 : 1;

	for (var i = 0; i < l.length && i < r.length; ++i) {
		var left = parseInt(l[i]), right = parseInt(r[i]);
		if (left != right)
			return ((left < right) ? ret : -ret);
	}
	return 0;
}

$.fn.dataTableExt.oSort['ip-address-asc'] = function(a, b) {
	return $.fn.dataTableExt.oApi.fnSortByIp(a, b, true);
};

$.fn.dataTableExt.oSort['ip-address-desc'] = function(a, b) {
	return $.fn.dataTableExt.oApi.fnSortByIp(a, b, false);
};
