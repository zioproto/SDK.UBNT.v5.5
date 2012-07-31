var ready = false;
var data_timer = 0;
var max_data = 20;
var stats_data = {};
var last_uptime = 0;
var dev_uptime = 0;
var plots = [];

$(document).ready(refreshAll);

function refreshAll() {
	if (typeof reloadStatus == 'function')
		reloadStatus();
	reloadData();
}

function reloadData() {
	if (!document.getElementById('throughput'))
		return;

	$.ajax({
		url: "ifstats.cgi",
		cache: false,
		dataType: "json",
		success: updateGraphs
	});

	return false;
}

function update_all(id, data, rxbytes, txbytes) {
	data.add_values( [rxbytes, txbytes], dev_uptime - last_uptime);
	updateCanvas(id, data);
}

function normalize_max(val, ticks)
{
	var delta = val / ticks;
	var magn = Math.pow(10, Math.floor(Math.log(delta) / Math.LN10));
	var norm = delta / magn;
	var tick_size = 10;
	if(norm < 1.5) tick_size = 1;
	else if(norm < 2.25) tick_size = 2;
	else if(norm < 3) tick_size = 2.5;
	else if(norm < 7.5) tick_size = 5;
	tick_size *= magn;
	if (Math.floor(val / tick_size) * tick_size >= val)
	{
		return val;
	}
	return (Math.floor(val / tick_size) + 1) * tick_size;
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

function normalize_tick(val) {
	var tick = (Math.round(val * 100) / 100).toString(10);
	var pos = tick.lastIndexOf('.');
	if (pos != -1 && pos < tick.length - 2)
		tick = tick.substr(0, pos + 3);
	return tick;
}

function formatBPS(value)
{
	var unit;
	var power;
	if (Math.round(value) < 1024)
	{
		unit = "bps";
		power = 0;
	}
	else if (Math.round(value / 1024) < 1024)
	{
		value = value / 1024;
		unit = "kbps";
		power = 1;
	}
	else
	{
		value = value / 1024 / 1024;
		unit = "Mbps";
		power = 2;		
	}
	value = normalize_float(value);
	return [value, unit, power, ""+value+unit];
}

function getOptions(units, ymax) {
	var opts = {
		xaxes: [{
			ticks: max_data,
			tickFormatter: function(n) {
				return '';
			},
			min: 0,
			max: max_data
		}],
		yaxes: [{
			ticks: 8,
			tickFormatter: function(n) {
				return (n != 0) ? normalize_tick(n) : units + ' 0';
			},
			min: 0,
			max: ymax
		}],
		legend: {
			position: 'nw',
			backgroundOpacity: 0.4
		}
	};
	return opts;
}

function updateCanvas(id, data)
{
	if (!document.getElementById(id))
		return;

	var dp = data.get_plot_values();
	var rx = formatBPS(dp[3][0])[3];
        var tx = formatBPS(dp[3][1])[3];

	var d = [{
			data: dp[0][0],
			label: 'RX: ' + rx,
			color: '#2389C6'
		},
		{
			data: dp[0][1],
			label: 'TX: ' + tx,
			color: '#FF0000'
		}
	];

	var opts = getOptions(dp[1], dp[2]);
	if (plots[id]) {
		var plot = plots[id];
		$.extend(true, plot.getOptions(), opts);
		plot.setData(d);
		plot.setupGrid();
		plot.draw();
	}
	else {
		plots[id] = $.plot($("#"+id), d, opts);
	}
}

function PlotData(series_count, max_data) 
{
	this.max_data = max_data;
	this.last = new Array(series_count);
	this.data = new Array(series_count);
	this.shifted = new Array(series_count);
	
	for (var i=0; i < series_count; ++i)
	{
		this.last[i] = 0;
		this.data[i] = new Array();
		this.shifted[i] = false;
	}

	this.add_values = function (values, sec)
	{
		var last;
		if (sec <= 0)
		{
			return;
		}
		for (var i = 0; i < this.data.length; ++i)
		{			
			if (this.data[i].length >= this.max_data)
			{
				this.data[i].shift();
			}
			if (values[i])
			{
				last = parseFloat(values[i]);
				if (this.data[i].length == 0)
					this.last[i] = last;
                                if (last < this.last[i]) {
                                	if (this.last[i] > 0x7FFFFFFF)
                                        	this.data[i].push((0xFFFFFFFF - this.last[i] + last) / sec * 8); // bps
                                        else {
                                        	if ((this.last[i] + last) < 0x8FFFFFFF) //handle restart
        	                        		this.data[i].push(last / sec * 8); // bps                                                
	                                        else
        	                        		this.data[i].push((0x7FFFFFFF - this.last[i] + last) / sec * 8); // bps
                                             }
                                }
                                else
					this.data[i].push((last - this.last[i]) / sec * 8); // bps
				this.last[i] = last;
			}
			else
			{
				this.data[i].push(0);
			}
			if (!this.shifted[i] && this.data[i].length == 2)
			{
				this.data[i].shift();
                                this.shifted[i] = true;
			}
		}
	}

	this.get_plot_values = function()
	{
		var max = 100; // 100 bps
		var i, j;
		var tmp, format;
		var last = [];
		var result = [];
		for (i = 0; i < this.data.length; ++i) {
			result[i] = [];
			for (j = 0; j < this.data[i].length; ++j) {
				tmp = this.data[i][j];
				if (tmp > max)
					max = tmp;
				result[i][j] = [j, tmp]
			}
			last[i] = this.data[i][j-1];
		}
		format = formatBPS(max);
		max = normalize_max(format[0], 8);
		if (format[2] > 0) {
			for (i = 0; i < result.length; ++i) {
				for (j = 0; j < result[i].length; ++j)
					 result[i][j][1] /= Math.pow(1024, format[2]);
			}
		}
		return [result, format[1], max, last]; // data, unit, normalized value
	}
}

function buildHeaderRow(idx, two) {
	var data = [];
	data.push('<tr><td><span id="label' + idx + '"></span></td>');
	data.push(two ? '<td><span id="label' + (idx + 1) +'"></span></td></tr>' : '<td width="50%">&nbsp;</td></tr>')
	return data.join('');
}

function buildGraphRow(idx, two) {
	var data = [];
	data.push('<tr><td><div class="graph-border"><div id="canvas' + idx + '" class="graph-canvas"></div></div></td>');
	data.push(two ?
		'<td><div class="graph-border"><div id="canvas' + (idx + 1) + '" class="graph-canvas"></div></div></td></tr>' :
		'<td>&nbsp;</td></tr>');
	return data.join('');
}

function buildStatsObj(idx) {
	var obj = {};
	obj.canvas = "canvas" + idx;
	obj.data = new PlotData(2, max_data);
	return obj;
}

function initGraphs(d) {
	var ifcount = d.interfaces.length;
	stats_data.length = ifcount;
	for (var i = 0; i < ifcount; i += 2) {
		var row = [];
		row.push(buildHeaderRow(i, i + 1 < ifcount));
		row.push(buildGraphRow(i, i + 1 < ifcount));
		$('#throughput > tbody').append(row.join(''));

		stats_data[d.interfaces[i].ifname] = buildStatsObj(i);
		$('#label' + i).text(devname2uidevname(d.interfaces[i].ifname));

		if (i + 1 < ifcount) {
			stats_data[d.interfaces[i+1].ifname] = buildStatsObj(i+1);
			$('#label' + (i + 1)).text(devname2uidevname(d.interfaces[i+1].ifname));
		}
	}
}

function updateGraphs(d) {
	if (!d) {
		if (typeof reloadStatus == 'function')
			reloadStatus();
		else 
			window.location.reload();
		return;
	}

	if (!stats_data.length)
		initGraphs(d);

	dev_uptime = d.host.uptime;
	for (var i = 0; i < d.interfaces.length; ++i) {
		var iface = d.interfaces[i];
		if (stats_data[iface.ifname]) {
			var stats = stats_data[iface.ifname];
			update_all(stats.canvas, stats.data,
				iface.stats.rx_bytes, iface.stats.tx_bytes);
		}
	}

	data_timer = setTimeout("reloadData()", ready ? 5000 : 500);
	ready = true;
	last_uptime = dev_uptime;
}

