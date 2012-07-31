var mainTable;
var _cols = {
	name: 0,
	rx: 4,
	tx: 6
};
var _initialized = false;

function handleError(xhr, textStatus, errorThrown) {
	if (xhr && xhr.status != 200 && xhr.status != 0)
		window.location.reload();
}

function kickEnd(data, textStatus, xhr) {
	$(this).enable();
	refreshIfList();
}

function refreshIfList() {
	$.ajax({
		cache: false,
		url: '/iflist.cgi',
		dataType: 'json',
		success: showIfList,
		error: handleError,
		complete: function(jqXHR, textStatus) {
			if (textStatus != "success")
				refreshIfList();
		}
	});
}

function refreshAll() {
	if (typeof reloadStatus == 'function')
		reloadStatus();
	refreshIfList();
}

function normalize_float(val) {
	if (!val)
		return 0;
	if (val > 100)
		return toFixed(val, 0);
	if (val > 10)
		return toFixed(val, 1);
	return toFixed(val, 2);
}

function formatBytes(value)
{
	var unit = "";
	if (Math.round(value) < 1024)
	{
		unit = "";
	}
	else if (Math.round(value / 1024) < 1024)
	{
		value = value / 1024;
		unit = "K";
	}
	else if (Math.round(value / 1024 / 1024) < 1024)
        {
		value = value / 1024 / 1024;
		unit = "M";
        }
	else
	{
		value = value / 1024 / 1024 / 1024;
		unit = "G";
	}
	value = normalize_float(value);
	return value+unit;
}

function getIfInfo(iface) {
	var result = [
		devname2uidevname(iface.ifname),
		iface.hwaddr,
		iface.mtu,
		iface.ipv4 ? iface.ipv4.addr : "0.0.0.0",
		iface.stats.rx_bytes,
		iface.stats.rx_errors,
		iface.stats.tx_bytes,
		iface.stats.tx_errors
	];

	return result;
}

function showIfList(ifl) {
	mainTable.fnClearTable();
	for (var i = 0; i < ifl.interfaces.length; ++i) {
		var iface = ifl.interfaces[i];
		var idx = mainTable.fnAddData(getIfInfo(iface), true);
	}
}

function tableCell(ctx, col) {
	var idx = mainTable.oApi._fnColumnIndexToVisible(mainTable.fnSettings(), col);
	if (idx == null)
		return $();
	return $('td', ctx).eq(idx);
}

$(document).ready(function() {
	if (_initialized) return;
	else _initialized = true;

	$.l10n.init({ dictionary: l10n_ifaces });
	$('#_refresh').click(refreshAll);
	$('#if_list th').removeClass('initial_hide');

	mainTable = $('#if_list').dataTable({
		'aaSorting': [ [0,'asc'] ],
		'bLengthChange': false,
		'bPaginate': false,
		'bFilter': false,
		'bInfo': false,
		'bSortClasses': false,
		'bAutoWidth': false,
		'oLanguage': {
			'sEmptyTable': $.l10n._('Loading, please wait...')
		},
		'fnInitComplete': function (oSettings) {
			oSettings.oLanguage.sEmptyTable = $.l10n._('No interface information.');
		},
		'fnRowCallback': function (nRow, aData, iDisplayIndex, iDisplayIndexFull) {
			$('td:eq(' + _cols.rx + ')', nRow).text(formatBytes(aData[_cols.rx]));
			$('td:eq(' + _cols.tx + ')', nRow).text(formatBytes(aData[_cols.tx]));
			return nRow;
		},
		'aoColumnDefs': [
			{ 'sType': 'string', 'aTargets': [ _cols.name ] }
		]
	});

	refreshIfList();
});
