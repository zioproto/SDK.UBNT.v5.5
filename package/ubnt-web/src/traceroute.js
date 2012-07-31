var traceroute_in_progress = false;
var conn = null;
var is_busy = false;
var current_sequence = 0;
var traceroute_url = "";
var traceroute_hop = 0;
var max_hops = 30;
var traceroute_form = null;

function setElementHtml(id, html) {
	var e = document.getElementById(id);
	if (e) {
		e.innerHTML = html;
		return true;
	}
	return false;
}

function addRow(table_id, columns) {
	var i;
	var tbody = document.getElementById(table_id).getElementsByTagName("TBODY")[0];
	var row = document.createElement("TR");
	var td;
	var widths = new Array("15px", "170px", "90px", "");

	for (i = 0; i < columns.length; ++i) {
		td = document.createElement("TD");
		if (widths[i].length > 0) {
			td.style.width = widths[i];
			td.style.overflow = "hidden";
		}
		td.innerHTML = columns[i];
		row.appendChild(td);    	
	}
	tbody.appendChild(row);
	// Firefox hack - resizing container will resize contents properly.
	row.style.width = "100%";
	var scrollDiv = document.getElementById("scroll_results");
	if (scrollDiv)
	{
		scrollDiv.scrollTop = scrollDiv.scrollHeight;
	}	
}

function clearTable(table_id) {
	var i;
	var tbody = document.getElementById(table_id).getElementsByTagName("TBODY")[0];
	var rows = tbody.getElementsByTagName("TR");
	for (i = 0; i < rows.length;) {
		var index = rows[i].sectionRowIndex;
		tbody.deleteRow(index);
	}
}

function addResult(hop, host, ip_addr, responses) {
	addRow("tr_results",
		new Array(hop, host, ip_addr, responses));
}

function handleResponse(httpRequest) {
	var rc = -255;
	try {
		if (httpRequest && httpRequest.status != 200) {
			return -2;
		}
		if (httpRequest && httpRequest.status == 200) {
			results = httpRequest.responseText.split('|');
			if (results.length > 0) {
				rc = parseInt(results[0]);
			}
		}
	} catch (e) {
		//just eat exception if any
	}
	if (rc || results.length != 8) {
		//error
		addResult("x", traceroute_l10n_fail, "x", "x");
		return -1;
	}
	addResult(results[1],
			'<input type="text" readonly style="border: none; width: 170px; background-color: transparent;" value="'+results[2]+'">',
			results[3],
			results[4]+" &middot; "+results[5]+" &middot; "+ results[6]);
	return (parseInt(results[7]) != 1) ? 0 : -1;
}

function tracerouteAction(sequence) {
	var local_sequence = sequence;
	is_busy = true;
	if (!conn.get(getTracerouteUrl(),
		function (httpRequest) {
			if (local_sequence != current_sequence) {
				return;
			}
			is_busy = false;
			if (!traceroute_in_progress) {
				return;
			}
			var rc = handleResponse(httpRequest);
			if (rc == 0 && traceroute_hop < max_hops) {
				doTraceroute();
			} else {
				stopTraceroute();
				if (rc == -2) {
					window.location.reload();
				}
			}
        	} ))
	{
		is_busy = false;
		addResult("x", "x", "x", traceroute_l10n_msg_unable_initialize);
		stopTraceroute();
		return false;
	}	
	return true;
}

function getTracerouteUrl() {
	traceroute_hop++;
	return traceroute_url + "&hop="+traceroute_hop+"&q="+Math.random();
}

function initTraceroute() {
	if (conn) {
		// abort just in case
		abortConnection()
	} else {
		conn = new ajax();
	}
	var dst_host = traceroute_form.dst_host.value;
	if (conn == null || dst_host.length == 0) {
		return false;
	}        
	var resolve = traceroute_form.resolve.checked;

	var query_string = "action=traceroute";
	query_string = query_string + "&dst_host="+dst_host;
	if (resolve) {
		query_string = query_string + "&resolve=1";    	
	}
	traceroute_url = "traceroute_action.cgi?"+query_string;

	traceroute_hop = 0;
	return true;
}

function doTraceroute() {
	if (traceroute_in_progress)
	{
		abortConnection();
		tracerouteAction(current_sequence);
	}
}

function startTraceroute(form) {
	traceroute_form = form;
	if (traceroute_in_progress) {
		return false;
	}
	if (!initTraceroute()) {
		return false;
	}
	traceroute_in_progress = true;
	tracerouteStarted();
	doTraceroute();
	return true;
}

function abortConnection() {
	if (conn && is_busy)
	{
		conn.abort();
		is_busy = false;
		return true;
	}
	return false;
}

function stopTraceroute() {
	traceroute_in_progress = false;
	current_sequence++;
	abortConnection();
	tracerouteStopped();
}

function _traceroute(f) {
	if (traceroute_in_progress) {
		stopTraceroute();
	} else {
		startTraceroute(f);
	}
}

function tracerouteStarted() {
	var o = document.getElementById("tr_start");
	if (o) { o.value=l10n_stop; }
	setDisabled(traceroute_form.dst_host, true);
	setDisabled(traceroute_form.resolve, true);
	clearTable("tr_results");
}

function tracerouteStopped() {
	var o = document.getElementById("tr_start");
	if (o) { o.value=l10n_start; }
	setDisabled(traceroute_form.dst_host, false);
	setDisabled(traceroute_form.resolve, false);
}
