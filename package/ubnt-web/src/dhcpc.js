var dhcpc = {
ifname : null,
warning : "Warning: DHCP client information release may terminate connection to your device.",
error: function(xhr, textStatus, errorThrown) {
	if (xhr && xhr.status != 200 && xhr.status != 0) {
		window.location.reload();
	}
},
init: function() {
	$('#dhcpcinfo')
		.delegate('input.dhcpcRenew', 'click', function() {
			dhcpc.renew($.tmplItem(this).data.ifname);
		})
		.delegate('input.dhcpcRelease', 'click', function() {
			dhcpc.release($.tmplItem(this).data.ifname);
		})
},
fetch : function(t) {
	dhcpc.fetch.success = function(data, textStatus, xhr) {
		var $tbody = $('#dhcpcinfo > tbody');
		if (data.dhcpc.status == 0) {
			$('#dhcpcTmpl')
				.tmpl(data.dhcpc.info)
				.appendTo($tbody.empty());
		}
		else
			$tbody.empty().append(
				$('<tr>').append(
					$('<td>').text(data.dhcpc.error)
				)
			);
	};
	$.ajax({
		url: '/dhcpcinfo.cgi',
		cache: false,
		async: t ? false : true,
		dataType: 'json',
		success: dhcpc.fetch.success,
		error: dhcpc.error
	});
	return false;
},
release : function(ifname) {
	dhcpc.release.success = function(data, textStatus, xhr) {
		dhcpc.fetch();
	};
	if (!confirm(dhcpc.warning))
        	return;
	$.ajax({
		url: '/dhcpcinfo.cgi?action=release&ifname=' + ifname,
		cache: false,
		dataType: 'json',
		success: dhcpc.release.success,
		error: dhcpc.error
	});
	return false;
},
renew : function(ifname) {
	dhcpc.renew.success = function(data, textStatus, xhr) {
		dhcpc.fetch();
	};
	$.ajax({
		url: '/dhcpcinfo.cgi?action=renew&ifname=' + ifname,
		cache: false,
		dataType: 'json',
		success: dhcpc.renew.success,
		error: dhcpc.error
	});
	return false;
}
};
