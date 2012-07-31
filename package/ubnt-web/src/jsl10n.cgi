#!/sbin/cgi
<?
header("Content-type: text/javascript");
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
>
l10n_start = "<? echo dict_translate("Start"); >";
l10n_stop = "<? echo dict_translate("Stop"); >";

jsval_l10n_err_form = "<? echo dict_translate("msg_missing_values|Please enter/select values for the following fields:"); >";
jsval_l10n_err_select = '<? echo sprintf(dict_translate("msg_invalid_select|Please select a valid \"%s\""), "%FIELDNAME%"); >';
jsval_l10n_err_enter = '<? echo sprintf(dict_translate("msg_invalid_enter|Please enter a valid \"%s\""), "%FIELDNAME%"); >';

pingtest_l10n_timeout = "<? echo dict_translate("timeout"); >";

system_l10n_change = "<? echo dict_translate("Change"); >";
system_l10n_upload = "<? echo dict_translate("Upload"); >";

traceroute_l10n_msg_unable_initialize = "<? echo dict_translate("Unable initialize request"); >";
traceroute_l10n_fail = "<? echo dict_translate("Fail"); >";

var js_translations = [];

js_translations['ON'] = "<? echo dict_translate("ON"); >";
js_translations['OFF'] = "<? echo dict_translate("OFF"); >";
js_translations['bridge'] = "<? echo dict_translate("bridge"); >";
js_translations['router'] = "<? echo dict_translate("router"); >";
js_translations['day(-s)'] = "<? echo dict_translate("day(-s)"); >";
js_translations['WPA'] = "<? echo dict_translate("WPA"); >";
js_translations['WPA-TKIP'] = "<? echo dict_translate("WPA-TKIP"); >";
js_translations['WPA-AES'] = "<? echo dict_translate("WPA-AES"); >";
js_translations['WPA2'] = "<? echo dict_translate("WPA2"); >";
js_translations['WPA2-TKIP'] = "<? echo dict_translate("WPA2-TKIP"); >";
js_translations['WPA2-AES'] = "<? echo dict_translate("WPA2-AES"); >";
js_translations['WEP'] = "<? echo dict_translate("WEP"); >";
js_translations['none'] = "<? echo dict_translate("none"); >";

/* antennas */
js_translations['Unknown'] = "<? echo dict_translate("Unknown"); >";
js_translations['Main'] = "<? echo dict_translate("Main"); >";
js_translations['Secondary'] = "<? echo dict_translate("Secondary"); >";
js_translations['Diversity'] = "<? echo dict_translate("Diversity"); >";
js_translations['Vertical'] = "<? echo dict_translate("Vertical"); >";
js_translations['Horizontal'] = "<? echo dict_translate("Horizontal"); >";
js_translations['Adaptive'] = "<? echo dict_translate("Adaptive"); >";
js_translations['External'] = "<? echo dict_translate("External"); >";
js_translations['Internal'] = "<? echo dict_translate("Internal"); >";
js_translations['Antenna 1'] = "<? echo dict_translate("Antenna 1"); >";
js_translations['Antenna 2'] = "<? echo dict_translate("Antenna 2"); >";

js_translations['err_invalid_mac'] = "<? echo dict_translate("err_invalid_mac|Invalid MAC Address"); >";
js_translations['err_too_many_macs'] = "<? echo dict_translate("err_too_many_macs|Too many MAC Addresses"); >";
js_translations['err_mac_exists'] = "<? echo dict_translate("err_mac_exists|MAC exists already"); >";

js_translations['err_invalid_freq'] = '<? echo dict_translate("err_invalid_freq|Invalid frequency selected - must be one from the Frequency List or \"Auto.\""); >';
js_translations['err_freq_list_status'] = '<? echo dict_translate("err_freq_list_status|Frequency List can not be disabled while <a href=\"ubnt.cgi\">airSelect</a> is turned on."); >';

js_translations['auto'] = "<? echo dict_translate("Auto"); >";
js_translations['N/A'] = "<? echo dict_translate("N/A"); >";

js_translations['hidden_ssid'] = "<? echo dict_translate("Hidden SSID:") >";

function jsTranslate(word)
{
	if (js_translations[word])
	{
		return js_translations[word];
	}
	return word;
}
