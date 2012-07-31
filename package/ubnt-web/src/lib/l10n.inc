<?

Function get_dictionary_file $language, $language_name (
	if (substr($language,0,2) == "u/")
	{
		$result = "/etc/persistent/lang/"
			+ substr($language,2,strlen($language) - 2);
	}
	else
	{
		$result = "lib/lang/" + $language + "/" + $language_name;
	}
	return $result;
);

Function get_language_options $languages, $active_language (
	$lang_options = "";
	reset($languages);
	$n = count($languages);
	$i = 0;
	while ($i < $n)
	{
		$lang = key($languages);
		$lang_options += "<option value=\"" + $lang + "\"";
		if ($lang == $active_language)
		{
			$lang_options += " selected";
		}
		$lang_options += ">" + $languages[] + "</option>";
		$i++;
	}
	return $lang_options;
);

unset($languages);
$languages["en_US"] = "English";
if ($cfg && $cfg != -1) /* just in case if we don't have config */
{
	$config_language = cfg_get_def($cfg, "gui.language", "en_US");
}
else
{
	$config_language = "en_US";
}
$active_language = "en_US";

openDir("lib/lang");
$dir = readDir();
while ($dir)
{
	if (substr($dir,0,1) != ".")
	{
		$dirs[] = $dir;
	}
	$dir = readDir();
}
closeDir();
$n = count($dirs);
$i = 0;
while ($i < $n)
{
	openDir("lib/lang/" + $dirs[$i]);
	$lang = readDir();
	while($lang)
	{
		if (substr($lang,0,1) != ".")
		{
			$languages[$dirs[$i]] = $lang;
		}
		$lang = readDir();
	}
	closeDir();
	$i++;
}
@openDir("/etc/persistent/lang");
$dir = @readDir();
while ($dir)
{
	if (substr($dir,0,1) != ".")
	{
		$languages["u/"+$dir] = $dir+" (U)";
	}
	$dir = readDir();
}
closeDir();

if (isset($languages[$ui_language]))
{
	$active_language = $ui_language;
}
elseif (isset($languages[$config_language]))
{
	$active_language = $config_language;
}

if (!$l10n_no_cookies)
{
	setcookie("ui_language", $active_language, 2147483647); /* last valid 32 bit time_t */
}
if (!$l10n_no_load)
{
	$result = dict_load(get_dictionary_file($active_language,
		$languages[$active_language]));
}

Function get_title $cfg, $title (
	global $board_name;
	global $board_subtype;
        $result = $board_name;
	if (strlen($board_subtype) > 0)
	{
		$result += " " + $board_subtype ;
	}
        if ($cfg != -1) {
		$result = cfg_get_def($cfg, "resolv.host.1.name", "$result");
        }
        $result = "[" + $result + "] - " + $title;
	return $result;
);
>
