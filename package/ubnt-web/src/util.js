function openPage(url, width, height) {
	if (width == undefined)
		width = 600;
	if (height == undefined)
		height = 400;
	var w = screen.width / 2 - width / 2;
	var h = screen.height / 2 - height / 2;
	var specs = 'location=no,status=no,toolbar=no,scrollbars=yes,resizable=yes';
	specs += ',width=' + width + ',height=' + height;
	specs += ',left=' + w + ',top=' + h;

	var child = window.open(url, '_blank', specs);
	return false;
}
function selectOption(select, value) {
	if (select==null || value==null)
		return null;
	var strValue = typeof value == "string" ? value : value.toString();
	for(i = 0; i < select.options.length; i++) {
		var o = select.options[i];
		if (o.value.toUpperCase() == strValue.toUpperCase()) {
			o.selected = true;
			return o;
		}
	}
	return null;
}
function cache_images(urls) {
	if (document.images && urls) {
		for (i = 0; i < urls.length; ++i) {
			var img = new Image();
			img.src='FULL_VERSION_LINK/images/' + urls[i];
		}
	}
}
function addOption(select, text, value) {
	new_option = new Option(text, value);
	select.options[select.options.length] = new_option;
	return new_option;
}
function selectRadio(name, value) {
	var a = document.getElementsByName(name);
	if (a == null)
		return;
	var i = 0;
	for (; i < a.length; ++i) {
		var o = a[i];
		if (o.value == value) {
			o.checked = true;
		}
        }
}
function getRadioValue(name) {
	var a = document.getElementsByName(name);
	if (a == null)
		return "";
	var i = 0;
	for (; i < a.length; ++i) {
		var o = a[i];
		if (o.checked) {
			return o.value;
		}
        }
	return "";
}
function statusClicked(c,ids) {
	var disabled=true;
	if (!c) return;
	if(c.checked){disabled=false;}
	for (x=0;x<ids.length;++x) {
		var d=document.getElementById(ids[x]);
		if(d) d.disabled=disabled;
	}
}
function changeDisplay(id,value) {
	var o=document.getElementById(id);
	if (o) o.style.display=value;
}
function changeDisplays(ids,value) {
	if (ids==null) return;
	for (x=0;x<ids.length;++x){
		changeDisplay(ids[x], value);
	}
}
function setDisabled(element, disabled) {
	if (!element) {
		return false;
	}
	element.disabled = disabled;
	return true;
}

function add_select_enter_submit(select_id, callback)
{
	var select = document.getElementById(select_id);
	if (!select)
	{
		return false;
	}
	select.onkeydown = function(e)
	{
		e = e || window.event;
        if (e.keyCode == 13)
        {
			callback();
			return false;
		}
	};
}

function showTool(select)
{
	if (select.value.length == 0)
		return;

	var height = 360;
	var width = 700;
	if (select.value.indexOf("survey.cgi") != -1)
		width += 100;
	else if (select.value.indexOf("pingtest.cgi") != -1)
		height += 10;
	else if (select.value.indexOf("discovery.cgi") != -1) {
		height = 620;
		width = 900;
	}

	openPage(select.value, width, height);
	select.selectedIndex = 0;
}

function secsToCountdown(seconds, day_str, days_str) {
	var days = (seconds / 86400) | 0;
	var hours = "" + (((seconds / 3600) | 0) % 24);
	var minutes = "" + (((seconds / 60) | 0) % 60);
	var secs = "" + (seconds % 60);

	var result = (hours.length < 2 ? "0" : "") + hours + ":"
			+ (minutes.length < 2 ? "0" : "") + minutes + ":"
			+ (secs.length < 2 ? "0" : "") + secs;

	if (days > 0) {
		result =  "" + days + " " + ((days == 1) ? day_str : days_str) + " " + result;
	}

	return result;
}


/*
	parseUri 1.2.1
	(c) 2007 Steven Levithan <stevenlevithan.com>
	MIT License
*/
function parseUri (str) {
	var	o   = parseUri.options,
		m   = o.parser[o.strictMode ? "strict" : "loose"].exec(str),
		uri = {},
		i   = 14;

	while (i--) uri[o.key[i]] = m[i] || "";

	uri[o.q.name] = {};
	uri[o.key[12]].replace(o.q.parser, function ($0, $1, $2) {
		if ($1) uri[o.q.name][$1] = $2;
	});

	return uri;
};

parseUri.options = {
	strictMode: false,
	key: ["source","protocol","authority","userInfo","user","password","host","port","relative","path","directory","file","query","anchor"],
	q:   {
		name:   "queryKey",
		parser: /(?:^|&)([^&=]*)=?([^&]*)/g
	},
	parser: {
		strict: /^(?:([^:\/?#]+):)?(?:\/\/((?:(([^:@]*):?([^:@]*))?@)?([^:\/?#]*)(?::(\d*))?))?((((?:[^?#\/]*\/)*)([^?#]*))(?:\?([^#]*))?(?:#(.*))?)/,
		loose:  /^(?:(?![^:@]+:[^:@\/]*@)([^:\/?#.]+):)?(?:\/\/)?((?:(([^:@]*):?([^:@]*))?@)?([^:\/?#]*)(?::(\d*))?)(((\/(?:[^?#](?![^?#\/]*\.[^?#\/.]+(?:[?#]|$)))*\/?)?([^?#\/]*))(?:\?([^#]*))?(?:#(.*))?)/
	}
};
/*end of parseUri 1.2.1*/
/*
 * depends on:
 *     ajax.js
 *
 * TODO: cache elements instead of querying them everytime
 */
function IPList(selectId,inputId,refreshImgId) {
	this.selectId = selectId;
	this.inputId = inputId;
	this.imgId = refreshImgId;
	this.started = false;
	this.conn = null;
	var me = this;

	this.refreshStarted = function() {
		var r = document.getElementById(me.selectId);
		if (r) setDisabled(r, true);
		r = document.getElementById(me.imgId);
		if (r) { r.src='FULL_VERSION_LINK/images/loading.gif'; }
		me.started = true;
	}
	this.refreshStopped = function() {
		var r = document.getElementById(me.selectId);
		if (r) setDisabled(r, false);
		r = document.getElementById(me.imgId);
		if (r) { r.src='FULL_VERSION_LINK/images/refresh.png'; }
		me.started = false;
	}
	this.triggerManual = function() {
		var s = document.getElementById(me.selectId);
		setDisabled(document.getElementById(me.inputId), (s.selectedIndex != 0));
	}
	this.update = function() {
		if (me.started) {
			return false;
		}
		me.refreshStarted();
		if (this.conn) {
			this.conn.abort();
		} else {
			this.conn = new ajax();
		}
		var s = document.getElementById(me.selectId);

		var oldVal = s.options[s.selectedIndex];
		if (!this.conn.get('ipscan.cgi?a='+new Date().getTime(),
			function(req) {
				var o = document.getElementById(me.selectId);
				if (o) {
					o.length = 1;
					if (req.status == 200) {
						var ips = req.responseText.split("\n");
						for (i = 0; i < ips.length; ++i) {
							var ip = ips[i];
							if (ip.length > 0) {
								addOption(o, ip, ip);
							}
						}
					}
					selectOption(o, oldVal.value);
				}
				me.refreshStopped();
			})) {
			me.refreshStopped();
		}

		return true;
	}

	this.update();
	this.triggerManual();

	var s = document.getElementById(this.selectId);
	s.onchange = this.triggerManual;
	s = document.getElementById(this.imgId);
	s.onclick = this.update;

	return this;
}

var toFixed = 
(function() {
	return toFixedString;

	function toFixedString(n, digits) {
		var unsigned = toUnsignedString(Math.abs(n), digits);
		return (n < 0 ? "-" : "") + unsigned;
	}

	function toUnsignedString(m, digits) {
		var t, s = Math.round(m * Math.pow(10, digits)) + "", start, end;
		if (/\D/.test(s)) {
			return "" + m;
		}
		s = padLeft(s, 1 + digits, "0");
		start = s.substring(0, t = (s.length - digits));
		end = s.substring(t);
		if (end) {
			end = "." + end;
		}
		return start + end; // avoid "0."
	}

	function padLeft(input, size, ch) {
		var s = input + "";
		while(s.length < size) {
			s = ch + s;
		}
		return s;
	}
})();

var fwUpdateCheck =
(function() {
	return checkForUpdate;

	function checkForUpdate(force, fw_check) {
		var now = $.now();
		var interval = 1000 * 60 * 60 * 24;
		var last_check = $.cookie("last_check");

		var do_check = force ||
			(fw_check.enabled && (!last_check || now - last_check > interval));

		if (do_check) {
			$.cookie('last_check', now, { path: '/', expires: 1 });
			var data = {};
			if (force) 
				data["force"] = "yes";
			$.ajax({
				type: "GET",
				url: "/update_check.cgi",
				data: data,
				dataType: "json",
				cache: false,
				success: function(json) {
					if (!json || !json.update|| json.update != "true")
						return handleNoUpdate(force, fw_check);

					if (!json.date || !json.version)
						return handleNoUpdate(force, fw_check);

					var known = $.cookie('hide_update');
					if (!force && (known && json.date <= known))
						return;

					var r = new RegExp('\\{1\\}', 'g');
					var update_str = fw_check.fw_str.replace(r, json.version);
					createDiv(json.date, update_str, fw_check.dl_str);
					$("#notify-slide-container").slideDown('slow');
				}
			});
		}
	}

	function handleNoUpdate(force, fw_check) {
		if (!force)
			return;
		createDiv(null, fw_check.no_str, fw_check.dl_str);
		$("#notify-slide-container").slideDown('slow');
	}

	function createDiv(update_date, update_str, download_str) {
		$("#notify-slide-container").remove(); 

		var info = [];
		info.push('<div id="notify-slide-container">');
		info.push('<div id="notify-slide-content" class="rounded-corners">');
		info.push('<div id="notify-slide-text">');
		info.push('<p>' + update_str + '</p>');
		info.push('<p>' + download_str + '</p>');
		info.push('<p align="right">');
		info.push('<input id="hide-notify" type="button" value="Dismiss">');
		info.push('</p>');
		info.push('</div>');
		info.push('</div>');
		info.push('</div>');

		$("body").append(info.join(''));

		$("#hide-notify").click(function() {
			$("#notify-slide-container").slideUp("slow"); 
			if (update_date)
				$.cookie("hide_update", update_date);
		});
	}
})();
