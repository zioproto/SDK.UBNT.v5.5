#!/sbin/cgi
<?
$l10n_no_cookies = 1;
$l10n_no_load = 1;
include("lib/l10n.inc");

Function trim $str
(
	return ereg_replace("\s+$", '', ereg_replace("(^\s+)", '', $str));
);

if (substr($dictionary_file,0,2) == "u/")
{
	$is_uploaded = 1;
	$selected_name = substr($dictionary_file,2,strlen($dictionary_file) - 2);
}
else
{
	$is_uploaded = 0;
	$selected_name = $dictionary_file;
}

$modified = 0;
if ($REQUEST_METHOD=="POST" && isset($upload))
{
	$new_name = trim($new_name);
	if ($new_name == "")
	{
		$new_name = strrchr($file, "/");
		if ($new_name == "")
		{
			$new_name = $file;
		}
		else
		{
			$new_name = substr($new_name, 1, strlen($new_name) - 1);
		}
	}
	chmod($file, "644");
	@mkdir("/etc/persistent/lang/", 0755);
	exec("mv "+$file+" /etc/persistent/lang/"+$new_name);
	$modified = 1;
	$uploaded = 1;
}
elseif ($REQUEST_METHOD=="POST" && isset($rename) && $is_uploaded
	&& strchr($selected_name, "/") == "" && strchr($new_name, "/") == "")
{
	$new_name = trim($new_name);
	if ($new_name != "" && $dictionary_file != "")
	{
		rename(get_dictionary_file($dictionary_file,
			$languages[$dictionary_file]), "/etc/persistent/lang/" + $new_name);
		$modified = 1;
	}
}
elseif ($REQUEST_METHOD=="POST" && isset($delete) && $is_uploaded
	&& strchr($selected_name, "/") == "")
{
	unlink(get_dictionary_file($dictionary_file, $languages[$dictionary_file]));
	$modified = 1;
}
elseif ($REQUEST_METHOD=="POST" && isset($download)
	&& strchr($selected_name, "/") == "")
{
	$download_file =
		get_dictionary_file($dictionary_file, $languages[$dictionary_file]);
	/* echo("Download file: " + $download_file + "<br>\n"); */
	ClearStatCache();
	if (fileSize($download_file) < 0)
	{
		$download_file = "English.txt";
	}

	$download_name = strrchr($download_file, "/");
	if ($download_name == "")
	{
		$download_name = $download_file;
	}
	else
	{
		$download_name = substr($download_name, 1, strlen($download_name) - 1);
	}

	header("Content-Type: application/force-download");
	header("Content-Disposition: attachment;filename=\""+$download_name+"\"");
	include($download_file);
	exit;
}

if ($modified)
{
	exec("cfgmtd -w -p /etc/");
	/* reload dictionary file */
	include("lib/l10n.inc");
}

?>
<html>
<head>
<title>Testing Language Translation Upload</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<style type="text/css">
body, td, th, table {
    font-family: Verdana, Arial, Helvetica, sans-serif;
    font-size: 12px;
}
th { border: 1px solid #aaa;
background-color: #eee;
color: #900;
text-align: left;
text-indent: 10px;
white-space: nowrap;
margin-left: 10px;
font-size: 14px;
font-weight: normal;
letter-spacing: 0.3em;
width: 100%;
}
td { padding-left: 20px; padding-right: 20px; padding-top: 5px }
table { border: 0px; width: 700px }
pre {
background-color: #FFFCE2;
border: 1px solid #666;
padding: 5px 5px 5px 5px;
}
input[type=submit], input[type=reset] {
background: #eee;
color: #222;
border: 1px outset #ccc;
padding: .1em .5em;
}
input[type=text], input[type=file]{ padding: .25em .5em }
input[type=text]:focus, input[type=password]:focus, input[type=file]:focus {
border: 1px solid #886
}
input[type=text], input[type=file]{
background: #fff;
color: #000;
border: 1px solid #d7d7d7;
}
</style>
<script type="text/javascript" src="FULL_VERSION_LINK/js/jquery.js"></script>
<script type="text/javascript">
//<!--

function change_dictionary()
{
	var val = jQuery("#dictionary_file").val();
	if (!val)
		return false;
	if (val.substr(0,2) == "u/")
	{
		jQuery(":submit.modifies").show();
	}
	else
	{
		jQuery(":submit.modifies").hide();
	}
}

function init()
{
	jQuery("#dictionary_file").bind("change", change_dictionary).trigger("change");

}

jQuery(document).ready(init);

//-->
</script>
</head>
<body bgcolor="white">
<a href="index.cgi">Main</a>
<form enctype="multipart/form-data" action="<?echo $PHP_SELF;?>" method="POST">
<table align="center">
<tr><th colspan="2">Dictionary Files Testing</th></tr>
<tr><td>
Dictionaries:
<select id="dictionary_file" name="dictionary_file">
<? echo get_language_options($languages, $active_language); >
</select>
</td>
<td>
<input type="submit" id="download" name="download" value="Download">
<input class="modifies" type="submit" id="delete" name="delete" value="Delete">
</td></tr>
<tr><td>
New name:<input type="text" name="new_name" value="">
</td>
<td>
<input  class="modifies" type="submit" id="rename" name="rename" value="Rename"><br>
</td></tr>
<tr><td>
File: <input name="file" type="file"><br>
</td>
<td>
<input type="submit" name="upload" value="Upload"><br>
</td></tr>
<?
if ($uploaded)
{
    echo("<tr><td colspan=\"2\"><pre>\n");
    echo("Uploaded file: " + htmlspecialchars($file) + "\n");
    echo("Uploaded file size: " + $file_size);
    echo("\n</pre></td></tr>");
}
>
</table>
</form>
</body>
</html>
