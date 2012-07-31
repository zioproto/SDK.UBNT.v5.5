function ack2meters(ack, min) {
	/*
	Units of ack are uSeconds and ack is approximately 2x distance
	(plus some negligible offset accounted for by the minimum ack timeout which covers 0-300 meters or so)
	So the speed of light is ~300 meters/microsecond and ack timeout is roundtrip so
	we can take time_uS * 300 / 2 or time_uS * 150;
	extra_ack_uS * 150;
	*/
	if (ack < min)
		return 0;

	return (ack - min) * 150;
}

function AckController(ack, dst, slider) {
	this.ack = ack;
	this.dst = dst;
	this.slider = slider;
	this.maximum = 0;

	ack.controller = this;
	dst.controller = this;
	slider.controller = this;

	this.setMaximum = function(decimiles) {
		this.maximum = decimiles;
	}

	this.updated = function(obj, decimiles) {
		if (decimiles < 0)
			decimiles = 0;
		else if(decimiles > this.maximum)
			decimiles = this.maximum;

		this.slider._isChanging = true;
		this.slider.setValue(Math.round(decimiles));
		this.slider._isChanging = false;

		this.dst.setValue(Math.round(decimiles));

		this.ack.setValue(decimiles);
	}
}

function createSlider(minacktimeout, maxacktimeout) {
	var kilometers_per_mile = 1.609344;
	var slider = new Slider(document.getElementById("slider-1"),
		document.getElementById("slider-input-1"));
	var ack = document.getElementById("acktimeout");
	var dst = document.getElementById("distance");

	var controller = new AckController(ack, dst, slider);

	/* Will operate with decimile(0.1mile) steps:

	   kilometers = meters / 1000
	   miles      = kilometers / kilometers_per_mile
	   decimiles = miles * 10
	 */
	controller.setMaximum(ack2meters(maxacktimeout, minacktimeout)
		/100/kilometers_per_mile);
	slider.setMaximum(Math.round(ack2meters(maxacktimeout, minacktimeout)
		/100/kilometers_per_mile));

	slider.onchange = function() {
		if (!this._isChanging)
			this.controller.updated(this, this.getValue());
	};

	dst.slider = slider;
	dst.onchange = function() {
		var miles = parseFloat(this.value);
		var decimiles;
		if (isNaN(miles))
			miles = 0;

		decimiles = Math.floor(miles * 10);
		this.controller.updated(this, decimiles);
	};
	dst.distkm = document.getElementById("distkm");

	dst.setValue = function(decimiles) {
		this.value = decimiles / 10;
		if (this.distkm)
			this.distkm.innerHTML = Math.round(decimiles * kilometers_per_mile) / 10;
	};

	ack.slider = slider;
	ack.distance = document.getElementById("ackdistance");
	ack.setValue = function(decimiles) {
		var dist_m = Math.round(decimiles * 100 * kilometers_per_mile);
		this.value = Math.round(dist_m/150) + minacktimeout;
		if (this.distance)
			this.distance.value = Math.round(dist_m/150) * 150;
	}

	ack.onchange = function() {
		var acktime = parseInt(this.value);
		var decimiles;
		if (isNaN(acktime))
			acktime = minacktimeout; // Minimum value...

		decimiles = ack2meters(acktime, minacktimeout)/100/kilometers_per_mile;
		this.controller.updated(this, decimiles);
	};

	ack.onchange();
}

function createTxPowerSlider(maxPower) {
	var slider = new Slider(document.getElementById("slider-2"),
		   document.getElementById("slider-input-2"));
	var power = document.getElementById("txpower");
	/* Prevent slider from capping value before init_power is called
	and min/max are updated based on power mode and regulatory
	limits */
	slider.setMinimum(-40);
	slider.setMaximum(40);
	slider.setMaxMarker(40);

	slider.onchange = function () {
		$('#txpower').val(this.getValue());
	};

	power.slider = slider;
	power.onchange = function () {
		var intVal = parseInt(this.value);
		if (isNaN(intVal)) intVal = 0;
		this.slider.setValue(intVal);
	};
	power.onchange();
}

function setESSID(item, chwidth, lock) {
	var essid = $("#essid");
	var apmac = $("#apmac");
	if (item.essid.length > 0) {
		essid.val(item.essid);
		apmac.val(!lock ? '' : item.mac);
	}
	else {
		essid.val('any');
		apmac.val(item.mac);
	}

	var curr_val = $('#clksel_select').val();
	if (curr_val != chwidth && curr_val != 'E') {
		$('#clksel_select').val(chwidth == '0' ? 'E' : chwidth);
		$('#clksel_select').change();
	}

	var sec = item.encryption.toLowerCase();
	if (sec == '-') sec = 'none';
	if (sec == 'wpa' || sec == 'wpa2') {
		if (item.pairwise_ciphers.indexOf('CCMP') >= 0)
			sec += 'aes';
		else if (item.pairwise_ciphers.indexOf('TKIP') >= 0)
			sec += 'tkip';
	}

	$('#security').val(sec);
	$('#security').change();

	var auth = 'WPA-PSK';
	if (item.auth_suites.toLowerCase().indexOf('802.1x') >= 0)
		auth = 'WPA-EAP';

	$('#wpa_auth').val(auth);
	$('#wpa_auth').change();

	if (item.htcap == 0) {
		$('#rate_auto').attr('checked', true);
		$("#rate option:eq(0)").attr("selected", "selected");
	}
}

function ieee_mode_fixer(ieee_mode, clksel) {
	this.ieee_mode = ieee_mode.toLowerCase();
	if (clksel == "") clksel = "0";
	this.clksel = clksel;

	// TODO: consider about: "gt"
	this.isATurbo = function() {
		return this.ieee_mode == "at" || this.ieee_mode == "ast";
	}

	this.getIEEEMode = function() {
		if (this.isATurbo())
			return "a";

		//11nght40plus
		if (this.ieee_mode.length > 6)
			return this.ieee_mode.substr(0, 6);

		return this.ieee_mode;
	}

	this.getClkSel = function() {
		if (this.isATurbo())
			return "T";

		if (this.ieee_mode.length >= 8 && this.ieee_mode.substr(6,2) == "40")
			return "E";

		return this.clksel;
	}

	this.getExtChannel = function() {
		var ext = null;
		if (this.ieee_mode.length >= 8 && this.ieee_mode.substr(6,2) == "40") {
			ext = this.ieee_mode.substr(8);
			if (ext != "minus" && ext != "plus")
				ext = "";
		}
		return ext;
	}
}

var rates_cck  = [1, 2, 5.5, 11];
var rates_ofdm = [6, 9, 12, 18, 24, 36, 48, 54];
/** 11n rates tables are for a single chain.
 * Other chains table can/will be generated from this */
var rates_ht20 = [6.5, 13, 19.5, 26, 39, 52, 58.5, 65];
var rates_ht40 = [15, 30, 45, 60, 90, 120, 135, 150];

function hasCCK(ieee_mode) {
	return (ieee_mode == "b" || ieee_mode == "g" || ieee_mode == "pureg");
}

function hasOFDM(ieee_mode) {
	return (ieee_mode == "a" || ieee_mode == "g" || ieee_mode == "pureg");
}

function hasHT(ieee_mode) {
	return (ieee_mode == "11naht" || ieee_mode == "11nght");
}

function compareNum(a, b) {
	return a - b;
}

function init_rates(regdomain, ieee_mode, clksel, value, rate_id, select_max) {
	var select = document.getElementById(rate_id);
	if (!select)
		return value;

	var divider = 1;
	var multiplier = 1;
	var rates = [];
	var next_chain_rates;
	var i, j;

	/* make this work without regdomain */
	if (!regdomain || regdomain[getRegdomainMode(ieee_mode)]) {
		if(globals.chanbw == 0) {
			switch (parseInt(clksel)) {
				case 1:
					divider = 2;
					break;
				case 2:
					divider = 4;
					break;
				case 'E':
					multiplier = 1; /* do not multiply - E is HT40, will use rates_ht40 array */
					break;
				case 'T':
					multiplier = 2;
					break;
			}
		}

		if (hasCCK(ieee_mode) && clksel == 0)
			rates = rates.concat(rates_cck);

		if (hasOFDM(ieee_mode))
			rates = rates.concat(rates_ofdm);

		if (hasHT(ieee_mode)) {
			/* generate bitrates for all chains */
			for (i = 1;  i <= radio_chains;  i++) {
				next_chain_rates = rates.length;
				rates = (clksel == 'E') ? rates.concat(rates_ht40) : rates.concat(rates_ht20);
				for (j = next_chain_rates;  j < rates.length;  j++) {
					rates[j] = rates[j] * i;
				}
			}
		} else {
			rates.sort(compareNum);
		}
	}

	var options = select.options;
	options.length = 0;

	var num_rates = rates.length;

	var wmode = $('#wmode').val();
	var is_ap = (wmode == 'ap' || wmode == 'aprepeater');

	for (i = num_rates - 1; i >=0 ; i--) {
		var rate_value = hasHT(ieee_mode) ? i : rates[i];
		var rate1 = rates[i];
		var rate2 = 0;
		var rate_name = "MCS " + i + " - "; 
		if ((clksel == "E") && !is_ap) {
			var rate_per_chain = i % 8;
			var chain_nr = Math.floor(i/8) + 1;
			rate1 = rates_ht20[rate_per_chain] * chain_nr;
			rate2 = rates_ht40[rate_per_chain] * chain_nr;
		}
		rate1 *= (multiplier/divider);
		rate2 *= (multiplier/divider);
		if (globals.chanbw) {
			rate1 *= (globals.chanbw / 20.0);
			rate2 *= (globals.chanbw / 40.0);
		}
		rate_name += (Math.round(10.0 * rate1) / 10);
		if (rate2)
			rate_name += (" [" + (Math.round(10.0 * rate2) / 10) + "]");
		options[options.length] =
			new Option(rate_name, rate_value, false, value == rate_value);
	}
	if (select_max)
		select.selectedIndex = 0;

	$(document).trigger('rate_list_updated');
	return options.length ? select.options[select.selectedIndex].value : value;
}

function setWepEnabled(enabled, do_focus) {
	$('#wep_key_length, #wep_key_id, #wep_key, #wep_key_type').enable(enabled);
	if (do_focus && enabled)
		$('#wep_key').focus();
}

function setWpaEnabled(enabled, do_focus) {
	var key = $('#wpa_key');
	var auth = $('#wpa_auth');
	var auth_val = auth.val();
	key.disable(!enabled);
	auth.disable(!enabled);
	var is_psk = (auth_val && auth_val.toUpperCase() == "WPA-PSK");
	if (is_psk) {
		$('.i_wpaeap').disable();
		$('.i_wpapsk').disable(!enabled);
		$('.wpaeap').hide();
		$('.wpapsk').toggle(enabled);
	} else {
		$('.i_wpapsk').disable();
		$('.i_wpaeap').disable(!enabled);
		$('.wpapsk').hide();
		$('.wpaeap').toggle(enabled);
                toggleRadiusAcct();
	}
	if (do_focus && enabled) {
		if (is_psk) {
			key.focus();
		} else {
			$('.wpaeap .pwd').focus();
		}
	}
}

function setNoSecEnabled(enabled, do_focus) {
	var st = $('#radius_mac_acl_status').is(':checked');
        if (enabled) {
		$('.radmacacl').toggle(st);
                $('.i_radaclmac').disable(!st);
                $('.i_radaclmacform').disable(!st);
                $('.radaclmacform').toggle(st);
                toggleRadiusAcct();
	} else {
		$('.nosec').toggle(enabled);
                $('.i_radaclmacform').disable(!enabled);
                $('.radaclmacform').toggle(enabled);
	}
}

function setAuthTypeEnabled(enabled) {
	$('[name="authtype"]').enable(enabled);
}

function setMACClone(enabled) {
	var ctrl = $('#macclone').enable(enabled);
	if (!enabled)
		ctrl.attr('checked', false);
}

function setAutoWDS(enabled) {
	var ctrl = $('#wds_auto').enable(enabled);
	if (!enabled)
		ctrl.attr('checked', false);
}

function setWDSPeers(enabled) {
	var ctrls = $('[name^=peer]').enable(enabled);
	if (!enabled)
		ctrls.val('');
}

function chooseSecurity(select, do_focus) {
	var mode = select.value.toUpperCase();
	var wmode = document.getElementById("wmode");
	var wpa = false;
	var wep = false;
        var nosec = false;
	var authtype = false;
	var macclone = true;
	var wds = true;
	if (wmode != null)
		wmode = wmode.value.toLowerCase();
	if (mode == "WEP") {
		wpa = false;
		wep = true;
		authtype = true;
		updateWepError();
	} else if (mode.substring(0, 3) == "WPA") {
		wpa = true;
		wep = false;
		authtype = false;
		macclone = false;
		if (wmode == "aprepeater") {
			wds = false;
		}
	} else nosec = true;
	$('.nosec').toggle(nosec);
	$('.wpa').toggle(wpa);
	$('.wep').toggle(wep);
	setWpaEnabled(wpa, do_focus);
       	setWepEnabled(wep, do_focus);
       	setNoSecEnabled(nosec, do_focus);
	setAuthTypeEnabled(authtype);
	setMACClone(macclone);
	setAutoWDS(wds);
	setWDSPeers(wds);
}

function chooseWPA(select, do_focus) {
	var sec = $('#security').val().toUpperCase();
	var wpa = (sec.substring(0, 3) == "WPA");
        if (sec == "NONE")
        	setNoSecEnabled(true, do_focus);
        else
		setWpaEnabled(wpa, do_focus);
}

function chooseDiversity(diversity, name1, name2) {
	var o1 = document.getElementById(name1);
	var o2 = document.getElementById(name2);
	var disable = false;
	if (diversity.checked) {
		disable = true;
	}

	o1.disabled = disable;
	o2.disabled = disable;
}

var RADSecret_regex = /^[\x20-\x7e]*$/;
function validateRadiusSecret(id, name, value) {
	if (value != null && value.length > 0 && value.length < 33 && RADSecret_regex.exec(value) != null)
		return true;

	return false;
}

var HwAddr_regex = /^([0-9a-fA-F][0-9a-fA-F]:){5}([0-9a-fA-F][0-9a-fA-F])$/

function validateHwAddr(id, name, value) {
	if (id == "peer1") {
		if (!$('#wds_auto').is(':checked') && !value)
			return false;
	}
	if (value != null && value.length > 0 && HwAddr_regex.exec(value) == null)
		return false;

	return true;
}

var WEP64_regex = /^([0-9]|[a-f]|[A-F]){10}$/
var WEP128_regex = /^([0-9]|[a-f]|[A-F]){26}$/

function updateWepError() {
	var key = document.getElementById("wep_key");
	var keylen = document.getElementById("wep_key_length");
	var keytype = document.getElementById("wep_key_type");
	var len = 5;
	if (keylen.value == "wep128") {
		len = 13;
	}

	var type_err = "HEX pairs";
	if (keytype && keytype.value == "2") {
		type_err = "ASCII characters";
	}

	key.realname = "WEP key (" + len + " " + type_err + ")";
}

function validateWepKey(id, name, value) {
	var security = document.getElementById("security");
	if (security.value.toUpperCase() == "WEP") {
		var key = document.getElementById(id);
		var keylen = document.getElementById("wep_key_length");
		var keytype = document.getElementById("wep_key_type");
		var len = 10;
		var regex = WEP64_regex;
		if (keylen.value == "wep128") {
			len = 26;
			regex = WEP128_regex;
		}
		if (keytype && keytype.value == "2") {
			regex = null;
			if (len == 10) {
				len = 5;
			} else {
				len = 13;
			}
		}

		if (value == "" || value.length != len) {
			return false;
		}

		if (regex != null && regex.exec(value) == null) {
			return false;
		}
	}
	return true;
}

function validateWpaKey(id, name, value) {
	var security = document.getElementById("security");
	var upsec = security.value.toUpperCase();
	var auth = document.getElementById("wpa_auth");
	if (auth)
        	auth = auth.value.toUpperCase();
	if (upsec.substring(0, 3) == "WPA" && (auth != "WPA-EAP")) {
		if (value == "" || value.length < 8 || value.length > 63) {
			return false;
		}
		if (!/^[ -~]{8,63}$/.test(value))
		{
			return false;
		}
	}
	return true;
}

function validateWpaIdent(id, name, value) {
	security = $('#security').val().toUpperCase();
	var auth = $('#wpa_auth').val().toUpperCase();

	if (security.substring(0, 3) == "WPA" && auth == "WPA-EAP") {
		if (value == "" || value.length < 1 || value.length > 63)
			return false;

		if (!/^[ -~]{1,63}$/.test(value))
			return false;
	}
	return true;
}

function shaperStatusClicked() {
	var enabled = $('#shaper_status').is(':checked');
	$('.i_shaper').disable(!enabled);
	$('.shaper').toggle(enabled);
}

function getRegdomainMode(ieee_mode) {
	if (ieee_mode == "a" || ieee_mode == "11naht")
		return "A";

	if (ieee_mode == "b")
		return "B";

	//11nght
	return "G";
}

function checkChanShiftFlags(chanshift, channel) {
	var no_shift = !parseInt(chanshift);

	if (no_shift && !(channel[2] & 1))
		return false;

	if (!no_shift && !(channel[2] & 2))
		return false;

	return true;
}

function parse_full_regdomain(full_regdomain) {
	var i;
	var result;
	var mode;
	var freq;
	var clksel;
	var channel;
	var regdomain = [];
	var ext = 0;
	var shift_flags; // bitmask: 1 - "normal", 2 - "shifted"
	var has_dfs = false;

	// -1: workaround for explorer array termination (,) problem
	for (i = 1; i < full_regdomain.length - 1; i++)
	{
		result = full_regdomain[i].split(/\s+/);
		mode = result[2];
		freq = parseInt(result[0]);
		channel = freq;

		if (mode == "T")
		{
			if (freq < 3000 || freq >= 10000)
			{
				mode = "G";
				/* TODO: Skip 2GHz turbo for now */
				continue;
			}
			else
			{
				mode = "A";
			}
		}
		clksel = result[3];
		if (clksel == "20")
		{
			clksel = "0";
		}
		else if (clksel == "10")
		{
			clksel = "1";
		}
		else if (clksel == "5")
		{
			clksel = "2";
		}
		else
		{
			clksel = "T";
		}
		if (!regdomain[mode])
		{
			regdomain[mode] = [];
		}
		if (!regdomain[mode][clksel])
		{
			regdomain[mode][clksel] = [];
		}
		ext = parseInt(result[5]);
		if(isNaN(ext))
			ext = 0;

		if (!has_dfs && (ext & 9) == 9)
			has_dfs = true;

		if (parseInt(result[6]))
		{
			shift_flags = 2;
		}
		else
		{
			shift_flags = 1;
		}
		var reg = parseInt(result[7]);
		if (isNaN(reg))
			reg = 1;
		if (!regdomain[mode][clksel][channel])
		{
			regdomain[mode][clksel][channel] =
				[result[0], parseFloat(result[4]), shift_flags, ext, reg];
		}
		else
		{
			regdomain[mode][clksel][channel][2] |= shift_flags;
		}
		if ((ext & 3) == 3 || (ext & 5) == 5)
		{
			// Extended logic
			if (!regdomain[mode]['E'])
			{
				regdomain[mode]['E'] = [];
			}
			if (!regdomain[mode]['E'][channel])
			{
				regdomain[mode]['E'][channel] =
					[result[0], parseFloat(result[4]), shift_flags, ext, reg];
			}
			else
			{
				regdomain[mode]['E'][channel][2] |= shift_flags;
			}
		}
	}

	var rg_data = {
		regdomain: regdomain,
		has_dfs: has_dfs };

	return rg_data;
}

function selectToArray(select) {
	var arr = [];
	var i = 0;

	select.find('option').each(function() {
		arr[i] = [];
		arr[i].value = $(this).val();
		arr[i++].display = $(this).text();
	});

	return arr;
}

function arrayToSelect(select, items, validator) {
	var options = '';
	for (var i=0; i<items.length; ++i) {
		if (validator && !validator(i))
			continue;
		options += '<option value="' + items[i].value + '">' + items[i].display + '</option>';
	}

	select.html(options);
}

function filterRates(security) {
	var select = $('#rate');
	MIN_WEP_INDEX = 3;

	if (typeof filterRates.all == 'undefined')
		filterRates.all = selectToArray(select);

		var rates = selectToArray(select);
		var current = select.prop("selectedIndex");

		if (/^(wep|wpa|wpatkip|wpa2|wpa2tkip)$/.test(security)) {
			if (filterRates.all.length == rates.length) {
				arrayToSelect(select, filterRates.all, function(val) {
					return (val >= MIN_WEP_INDEX);
				});

			current = (current <= MIN_WEP_INDEX) ? 0 : current - MIN_WEP_INDEX;
			$("#rate option:eq(" + current + ")").attr("selected", "selected");
		}
	}
	else if (filterRates.all.length > rates.length) {
		arrayToSelect(select, filterRates.all);

		var auto = $('#rate_auto').is(':checked');
		if (!auto || auto && current > 0)
			current += MIN_WEP_INDEX;

		$("#rate option:eq(" + current + ")").attr("selected", "selected");
	}
}

function get_scan_channels(regdomain, ieee_mode, clksel, chanshift, obey, is_ap) {
	var i;
	var result = [];

	var mode = getRegdomainMode(ieee_mode);
	if (!regdomain[mode] || !regdomain[mode][clksel])
		return result;

	var channels = regdomain[mode][clksel];
	if (clksel == 'E' && regdomain[mode][0] && !is_ap) {
		/* append missing HT20 channels */
		var cht20 = regdomain[mode]["0"];
		o:for (i in cht20)
		{
			for (j in channels)
				if (channels[j][0] == cht20[i][0])
					continue o;
			channels.push(cht20[i]);
		}
	}

	for (i in channels) {
		if (!checkChanShiftFlags(chanshift, channels[i]))
			continue;
		if (obey && channels[i][4] == 0)
			continue;
		result[i] = channels[i][0];
	}

	result.sort();
	return result;
}

function init_sta_power(regdomain) {
	var mode;
	var clksel;
	var channel;
	var sta_power = [];
	var power;

	for (mode in regdomain)
	{
		sta_power[mode] = [];
		for (clksel in regdomain[mode])
		{
			for (channel in regdomain[mode][clksel])
			{
				power = regdomain[mode][clksel][channel][1];
				if (isNaN(power))
				{
					continue;
				}
				if (!sta_power[mode][clksel])
				{
					sta_power[mode][clksel] = [power];
				}
				else if (sta_power[mode][clksel][0] < power)
				{
					sta_power[mode][clksel][0] = power;
				}
			}
		}
	}
	return sta_power;
}

function getRegdomainClksel(clksel) {
	switch (clksel)
	{
	case "T":
	case "0":
	case "1":
	case "2":
		return clksel;
	default:
		return "0";
	}
}

function fix_turbo_settings(ieee_mode, clksel) {
	var hmode = document.getElementById("ieee_mode");
	var hclksel = document.getElementById("clksel");
	if (clksel == "T")
	{
		clksel = "0";
		ieee_mode = "ast";
	}
	if (hmode)
	{
		hmode.value = ieee_mode;
	}
	if (hclksel)
	{
		hclksel.value = clksel;
	}
}

function ChannelWidth(width) {

	var standardWidths = {
		'5'  : { clksel : '2' },
		'10' : { clksel : '1' },
		'20' : { clksel : '0' },
		'40' : { clksel : 'E', label : globals.i18n.ht40_label }
	};

	function getLabel(width) {
		if (standardWidths[width] && standardWidths[width].label)
			return standardWidths[width].label;
		else
			return '' + width + ' MHz';
	}

	function getClkSel(width) {
		if (standardWidths[width])
			return standardWidths[width].clksel;
		else {
			if (width < 6)
				return '2';
			if (width < 11)
				return '1';
			return '0';
		}
	}

	function getCfgVal(width) {
		return standardWidths[width] ? 0 : width;
	}

	this.width = width;
	this.clksel = getClkSel(width);
	this.label = getLabel(width);
	this.cfg = getCfgVal(width);
}

function getChannelWidths(chanbw) {
	var clksel_min = 1, clksel_max = 40;
	var hi = (chanbw / 0xFFFFFFFF) | 0;
	var lo = (chanbw & 0xFFFFFFFF);

	var widths = [];
	for (var i = clksel_max; i >= clksel_min; i--) {
		var supported;
		if (i <= 32)
			supported = lo & (1 << (i - 1));
		else
			supported = hi & (1 << (i - 32 - 1));

		if (supported)
			widths.push(new ChannelWidth(i));
	}

	return widths;
}

function init_clksel(regdomain, ieee_mode, value, chanbw) {
	var mode = getRegdomainMode(ieee_mode);
	if (!regdomain[mode])
		return value;

	var select = document.getElementById("clksel_select");
	if (!select)
		return value;

	var options = select.options;
	options.length = 0;
	
	var clksel = value, width = chanbw;
	var availableWidths = getChannelWidths(globals.supported_chanbw);
	$.each(availableWidths,
		function(idx, val) {
			if (regdomain[mode][val.clksel]) {
				var selected = (val.clksel == clksel) && (val.cfg == width);
				var opt = new Option(val.label, val.clksel, false, selected);
				options[options.length] = opt;
				$.data(opt, 'chanbw', val.cfg);
			}
		}
	);

	globals.chanbw = chanbw;
	return options.length ? select.options[select.selectedIndex].value : value;
}

function init_frequencies(regdomain, ieee_mode, clksel, chanshift, value, select_last) {
	var $chan_freq = $("#chan_freq");
        
        if ($chan_freq.is(":hidden")) return value;

	var mode = getRegdomainMode(ieee_mode);
	if (!regdomain[mode] || !regdomain[mode][clksel])
		return value;

	var chanlist = [];
	var channels = regdomain[mode][clksel];
	var obey = $("#obey_regulatory_checkbox").is(":checked");
	for (var i in channels) {
		if (!checkChanShiftFlags(chanshift, channels[i]) || (obey && channels[i][4] == 0))
			continue;
		chanlist.push(channels[i][0]);
	}
	chanlist.sort();

	var opts = [];
	opts.push('<option value="0">' + jsTranslate('auto') + '</option>');
	$.each(chanlist, function(idx, val) {
		opts.push('<option value="'+ val + '">' + val + '</option>');
	});

	var curr_freq = $chan_freq.val();
	$chan_freq.empty().append(opts.join(''));

	if (!select_last) {
		$chan_freq.val(value);
		if ($chan_freq.val() != value)
			$chan_freq.val(0);
	}
	else {
		$chan_freq.val(curr_freq);
		if ($chan_freq.val() != curr_freq) {
			var new_val = 0;
			for (var i = chanlist.length - 1; i >= 0 && new_val == 0; i--) {
				if (chanlist[i] <= curr_freq) {
					new_val = chanlist[i];
					found = true;
				}
			}
			$chan_freq.val(new_val);
		}
	}

	var ret = $chan_freq.val();
	$(document).trigger('frequencies_updated');
	return ret;
}

function get_channel_by_freq(channels, chan_freq) {
	if (!channels || !chan_freq)
		return null;

	var i;
	for (i in channels)
		if (channels[i][0] == chan_freq)
			return i;
	return null;
}

function setMaxrate(rates_id, placeholder_id) {
	var select = document.getElementById(rates_id);
	var maxrate_obj = document.getElementById('maxrate');
	if (!select || !select.length) {
		return false;
	}
	if (maxrate_obj) {
		maxrate_obj.innerHTML = select.options[select.length - 1].text;
	}
}

function getRegdomainPower(regdomain, sta_power, ieee_mode, clksel, chan_freq) {
	var mode = getRegdomainMode(ieee_mode);
	var power = txpower_max;
	var channel = null;
	var channels;

	//Compliance test, Canada #2516 and USA-11a #1981 no limits
        if (country == "511" || country == "124" || (country == "840" && is_A(ieee_mode))) {
        	obey = false;
        	return 100; //aka no limit
        }

	channels = regdomain[mode][clksel];
	channel = get_channel_by_freq(channels, chan_freq);

	if (channel == null) {
		if (sta_power[mode] && sta_power[mode][clksel]) {
			power = sta_power[mode][clksel][0];
		}
	} else if (regdomain[mode] && regdomain[mode][clksel]
		&& regdomain[mode][clksel][channel]) {
		power = regdomain[mode][clksel][channel][1];
	}
	return power;
}

function toggleObeyRegulatory() {
	reinit_form(3); // channel list might be changed

	var power = document.getElementById("txpower");
	if (!power)
		return false;

	power.minvalue = calc_min_txpower(radio_chains, txpower_offset, txpower_max, low_txpower_atten, low_txpower_limit);
	power.maxvalue = calc_max_txpower();
	power.setAttribute('minvalue', power.minvalue);
	power.setAttribute('maxvalue', power.maxvalue);

	if (power.minvalue > power.maxvalue)
		power.minvalue = power.maxvalue;

	power.slider.setMinimum(power.minvalue);
	power.slider.setMaximum(power.maxvalue);
	power.slider.setMaxMarker(power.maxvalue);

	if (power.value > power.maxvalue) {
		power.value = power.maxvalue;
		power.onchange();
	} else if (power.value < power.minvalue) {
		power.value = power.minvalue;
		power.onchange();
	}

	$(document).trigger('obey_toggled');
}

function init_power(regdomain, sta_power, ieee_mode, clksel, chan_freq) {
	txpower_regdomain_limit = getRegdomainPower(regdomain, sta_power, ieee_mode, clksel, chan_freq);
	toggleObeyRegulatory();
}

function onIEEEMode(select) {
	ieee_mode = select.options[select.selectedIndex].value;
	reinit_form(1);
}

function onClksel(select) {
	old_clksel = clksel;

	var opt = select.options[select.selectedIndex];
	clksel = opt.value;
	globals.chanbw = $.data(opt, "chanbw");
	$("#clksel").val(clksel);

	submitNoSave();
}

function onChanshift(select) {
	chanshift = select.options[select.selectedIndex].value;
	reinit_form(3);
}

function onFrequency(select) {
	chan_freq = select.options[select.selectedIndex].value;
	reinit_form(4);
}

function onExtChannel(select) {
	extchannel = select.options[select.selectedIndex].value;
	document.getElementById("extchannel").value = extchannel;
	reinit_form(5);
}

function onSecurity(select, do_focus) {
	chooseSecurity(select, do_focus);

	if (parseInt(radio_chains) == 1)
		/* single chain devices can use all rates */
		return;

	var old_rate = $('#rate').val();
	filterRates(select.value);

	var new_rate = $('#rate').val();
	if (old_rate != new_rate) {
		rate = new_rate;
		reinit_form(2, true);
	}
}

function getClkSelWeight(clksel) {
	switch(clksel) {
		case '2':
			return 1.0;
		case '1':
			return 2.0;
		default:
			return 4.0;
	}
}

function calculateNewRateMultiplier(old_clksel, new_clksel) {
	return getClkSelWeight(new_clksel)/getClkSelWeight(old_clksel);
}

function adjustMulticastRate(rate, multiplier, select_validator_id) {
	var i;
	var new_rate;
	var select = document.getElementById(select_validator_id);
	var hidden = document.getElementById("mcast_rate");

	if (!hidden || !select || !select.length) {
		return rate;
	}
	new_rate = adjustRate(rate, multiplier);
	rate = select.options[0].value;
	for (i = 0; i < select.length; i++) {
		if (new_rate == select.options[i].value) {
			rate = new_rate;
		}
	}
	hidden.value = rate;
	return rate;
}

function on_antenna_gain_change(e) {
	_gain = new Number($('#antenna_gain').val());
	if (isFinite(_gain)) { 
		antenna_gain = _gain.valueOf();
                if (antenna_gain > 40) //we do not support more than 40dBi gain
                	$('#antenna_gain').val(40);
                if (_gain < 0) //we do not support less than 0dBi gain
                	$('#antenna_gain').val(0);                
		if ($('#obey_regulatory_checkbox').is(':checked')) {
			var test_power = calc_max_txpower();
			if (test_power <= 0) {
				antenna_gain = calc_max_gain();
				$('#antenna_gain').val(antenna_gain);
			}
		}
		toggleObeyRegulatory();
	} else {
		$('#antenna_gain').val(0);
	}
}

function on_cable_loss_change(e) {
        _loss = new Number($('#cable_loss').val());
        if (isFinite(_loss)) {
		cable_loss = _loss.valueOf();
                if (cable_loss > 40) //we do not support more than 40dBi loss
                	$('#cable_loss').val(40);
                if (cable_loss < 0) //we do not support less than 0dBi loss
                	$('#cable_loss').val(0);                
		if ($('#obey_regulatory_checkbox').is(':checked')) {
			var test_power = calc_max_txpower();
			if (test_power <= 0) {
				cable_loss = calc_min_loss();
				$('#cable_loss').val(cable_loss);
			}
		}
	        toggleObeyRegulatory();
        } else {
        	$('#cable_loss').val(0);
        }
}

function requiresCE(country) {
	// from http://www.wellkang.com/abouteu.html
	var ceList = [
		40,      /* Austria */
		56,      /* Belgium */
		100,     /* Bulgaria */
		196,     /* Cyprus */
		203,     /* Czech Republic */
		208,     /* Denmark */
		233,     /* Estonia */
		246,     /* Finland */
		250,     /* France */
		276,     /* Germany */
		300,     /* Greece */
		348,     /* Hungary */
		352,     /* Iceland */
		372,     /* Ireland */
		380,     /* Italy */
		428,     /* Latvia */
		438,     /* Liechtenstein */
		440,     /* Lithuania */
		442,     /* Luxembourg */
		470,     /* Malta */
		528,     /* Netherlands */
		530,     /* Netherlands-Antilles */
		578,     /* Norway */
		616,     /* Poland */
		620,     /* Portugal */
		642,     /* Romania */
		703,     /* Slovakia */
		705,     /* Slovenia */
		724,     /* Spain */
		752,     /* Sweden */
		826      /* United Kingdom */
	];

	return ($.inArray(parseInt(country), ceList) != -1);
}

function requiresFCC(country) {
	var fccList = [
		84,   /* Belize */
		124,  /* Canada */
		188,  /* Costa Rica */
		214,  /* Dominican Republic */
		222,  /* El Salvador */
		320,  /* Guatemala */
		340,  /* Honduras */
		388,  /* Jamaica */
		484,  /* Mexico */
		530,  /* Netherlands Antilles */
		591,  /* Panama */
		630,  /* Puerto Rico */
		780,  /* Trinidad and Tobago */
		840,  /* United States */
		843   /* United States (new 5GHz) */
        ];

	return ($.inArray(parseInt(country), fccList) != -1);
}

function is_G(ieee_mode) {
	return ieee_mode.toUpperCase().indexOf("G") >= 0;
}

function is_A(ieee_mode) {
	return !is_G(ieee_mode);
}

function calc_max_txpower() {
	if (!$('#obey_regulatory_checkbox').is(':checked'))
		return txpower_max;

	var max_txpower = txpower_regdomain_limit - antenna_gain + cable_loss;
	var ieee_mode = $('#ieee_mode').val();
	if (requiresFCC(country)) {
        	if (country != "124" && is_G(ieee_mode) && antenna_gain >= 6)
				max_txpower = 32 - Math.ceil((antenna_gain - cable_loss) / 3);
                if ((country != "124" && country != "840") && is_A(ieee_mode) && antenna_gain > 23)
                		max_txpower = 53 - antenna_gain - cable_loss;
	}

	if (max_txpower > txpower_regdomain_limit)
		max_txpower = txpower_regdomain_limit;

	if (max_txpower > txpower_max)
		max_txpower = txpower_max;

	return max_txpower;
}

function calc_min_txpower(chains, txpower_offset, txpower_max, low_txpower_atten, low_txpower_limit) {
	var chain_dbm = 0;
	switch(chains) {
		case 2:
			chain_dbm = 3;
			break;
		case 3:
			chain_dbm = 5;
			break;
	}
	
	var min_txpower = 0;
	if (radio_low_txpower_mode) {
		min_txpower = txpower_max - low_txpower_atten - low_txpower_limit;
		if (min_txpower < (txpower_offset + chain_dbm - low_txpower_atten))
			min_txpower = txpower_offset + chain_dbm - low_txpower_atten;
	} else {
		min_txpower = txpower_offset + chain_dbm;
	}

	return min_txpower;
}

function calc_max_gain() {
	var txpow = parseInt($('#txpower').val());
	var gain = txpower_regdomain_limit - txpow + cable_loss;
	var ieee_mode = $('#ieee_mode').val();
	if (requiresFCC(country)) {
		if (is_G(ieee_mode) && gain >= 6)
			gain = 96 - txpow*3 + cable_loss;
                if (is_A(ieee_mode) && gain > 23)
                	gain = 53 - txpow + cable_loss;
	}
	return gain;
}

function calc_min_loss() {
	var txpow = parseInt($('#txpower').val());
	var gain = parseInt($('#antenna_gain').val());
	var ieee_mode = $('#ieee_mode').val();
	var loss = gain + txpow - txpower_regdomain_limit;
	if (requiresFCC(country)) {
		if (is_G(ieee_mode) && gain >= 6)
			loss = gain - 96 + txpow*3;
		if (is_A(ieee_mode) && gain > 23)
			loss = gain - 53 + txpow;
	}
	return loss;
}

function toggleMACACL() {
	var st = $('#mac_acl_status').is(':checked');
	$('.i_macacl').disable(!st);
	$('.macacl').toggle(st);
}

function toggleRadiusMACACL() {
	setNoSecEnabled(true, false);
}

function toggleRadiusAcct() {
	var acct_status = $('#radius_acct_status');
	var st = acct_status.is(':visible') && acct_status.is(':checked');
        $('.radacct').toggle(st);
        $('.i_radacct').disable(!st);
}

function doStaSubmit(form) {
        if ($("#cc", form).is(':disabled')) {
		if (!validateStandard(form, 'error'))
			return false;
	}
	return true;
}

function doAPSubmit(form) {
	mac_error_set = 0;
        if ($("#cc", form).is(':disabled')) {
		if (!validateStandard(form, 'error'))
			return false;

		if (!validateFrequency() || !validateFreqListStatus())
			return false;
	}
	return true;
}

function submitNoSave() {
	var controls = [ 'wpa_auth', 'wpa_eap', 'wep_key_length', 'wep_key_type', 'wep_key_id'];
	var this_form = $('#this_form');
	$.each(controls, function(index, value) {
		if ($('#' + this).is(':disabled'))
			this_form.append('<input type="hidden" name="' + this + '" value="' + $('#' + this).val() + '"/>');
		});
	$('#cc').enable();
	this_form.submit();
}

function onChangeAPSubmit() {
	submitNoSave();
}

function onStaChangeSubmit() {
	submitNoSave();
}

function onAntennaChange(ap) {
	if ($("#rate option:selected").index() == 0)
		$('#this_form').append('<input type="hidden" name="select_max_rate" value="1"/>');
	return ap ? onChangeAPSubmit() : onStaChangeSubmit();
}

function validateChannels(all_channels, channel_list) {
	if (channel_list.length == 0)
		return false;
	for (var i = 0; i < channel_list.length; ++i)
		if ($.inArray(channel_list[i], all_channels) == -1)
			return false;
	return true;
}

function validateChannelScanList(id, name, value) {
	if (!$('#channel_scan_list').is(':checked'))
		return true;

	var scan_channels = get_current_scan_channels(value);
	var all_channels = get_scan_channels(regdomain, ieee_mode, clksel, chanshift, obey);
	return validateChannels(all_channels, scan_channels);
}
function toggleScanChannels() {
	var status = $('#channel_scan_list').is(':checked');
	$('#scan_channels').toggle(status);
	$('#edit_scan_channels').toggle(status);
}

function openScanChannelSelect() {
    openScanChannelSelectEx("scan_channels");
}

function openScanChannelSelectEx(elemId) {
	var url = "scan_channels.cgi";
	url += "?ieee_mode="+ieee_mode;
	url += "&country="+country;
	url += "&clksel="+clksel;
	url += "&chanshift="+chanshift;
	url += "&obey="+obey;
	url += "&elemId="+elemId;
	var o = document.getElementById(elemId);
	url += "&scan_channels=" + $('#' + elemId).val();
	return openPage(url, 700);
}

function setScanChannels(elemId, channels) {
	$('#' + elemId).val(channels);
}

function get_current_scan_channels(value) {
	var scanlist_regex = /^[0-9, ]*$/;
	var list_tidy_regex = / /g;

	if (value == null || value.length == 0 || scanlist_regex.exec(value) == null)
		return [];

	var scan_channels = value.replace(list_tidy_regex, "");
	return scan_channels.split(",");
}

function validateFrequency() {
	if (!$('#channel_scan_list').is(':checked'))
		return true;

	var curr_freq = $('#chan_freq').val();
	var has_freq = (curr_freq == 0);
	if (!has_freq) {
		var scan_channels = get_current_scan_channels($('#scan_channels').val());
		for (i = 0; !has_freq && i < scan_channels.length; ++i)
			has_freq = (scan_channels[i] == curr_freq);
	}

	if (!has_freq)
		setError(jsTranslate('err_invalid_freq'));

	return has_freq;
}

function validateFreqListStatus() {
	if (polling && polling_fh && !$('#channel_scan_list').is(':checked')) {
		setError(jsTranslate('err_freq_list_status'));
		return false;
	}
	return true;
}

function updateFreqList() {
	var freqs = $('#chan_freq option').map(function(i, n) { return $(n).val(); }).get();
	var items = get_current_scan_channels($('#scan_channels').val());
	var new_items = [];

	for (var i = 0; i < items.length; ++i)
		if ($.inArray(items[i], freqs) != -1)
			new_items[new_items.length] = items[i];

	if (items.length != new_items.length)
		$('#scan_channels').val(new_items.join(','));
}
