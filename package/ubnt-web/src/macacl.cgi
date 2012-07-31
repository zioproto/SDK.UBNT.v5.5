#!/sbin/cgi
<?
include("lib/settings.inc");
include("lib/l10n.inc");
include("lib/link.inc");

  
if ($REQUEST_METHOD == "POST") {
	if (isset($macacl_data) && strlen($macacl_data) > 100) {
		if (cfg_put($cfg_file, $macacl_data) != 0) {
        		$error_msg = dict_translate("msg_cfg_save_failed|Failed to save changes. Try again.");
		}
		else {
			$saved = 1;
		}
        } else {
        	$error_msg = dict_translate("msg_cfg_invalid|Failed to save changes. Invalid configuration data received. Try again.");
	}
}

$cfg = @cfg_load($cfg_file);
if ($cfg == -1) {
         include("lib/busy.tmpl");
         exit;
}

><!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN"
 "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title><? echo get_title($cfg, dict_translate("MAC ACL")); ></title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta http-equiv="Pragma" content="no-cache">
<link rel="shortcut icon" href="FULL_VERSION_LINK/favicon.ico" >
<link href="FULL_VERSION_LINK/jquery-ui.css" rel="stylesheet" type="text/css">
<link href="FULL_VERSION_LINK/style.css" rel="stylesheet" type="text/css">
<script type="text/javascript" language="javascript" src="jsl10n.cgi"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/jsval.js"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/util.js"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/js/jquery.js"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/js/jquery.utils.js"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/js/jquery.tmpl.js"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/network.js"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/firewall.js"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/ubnt.core.js"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/ubnt.cfg.js"></script>
<script type="text/x-jquery-tmpl" id="macaclTmpl">
        <tr>
                <td align="middle"><input type="checkbox" class="enableMacacl" {{if $data.enabled()}}checked{{/if}}/></td>
                <td>${mac}</td>
                <td>${comment} {{if !$data.comment || !$data.comment.length}}&nbsp;{{/if}} </td>
		<td>
			<input type="button" class="editMacacl" value="<? echo dict_translate("Edit"); >" />
			<input type="button" class="delMacacl" value="<? echo dict_translate("Del"); >" />
		</td>
        </tr>
</script>
<script type="text/x-jquery-tmpl" id="macaclEditTmpl">
        <tr>
                <td align="middle"><input type="checkbox" class="enableMacacl" {{if $data.enabled()}}checked{{/if}}/></td>
                <td>${mac}</td>
                <td><input type="text" id="edtMacaclComment" value="${comment}" /></td>
		<td>
			<input type="button" class="saveMacacl" value="<? echo dict_translate("Save"); >" />
			<input type="button" class="delMacacl" value="<? echo dict_translate("Del"); >" />
		</td>
        </tr>
</script>
<script type="text/javascript" language="javascript">
//<!--
<? if ($saved) { >
if (window.opener && !window.opener.closed && window.opener.doSubmit)
	window.opener.doSubmit();
window.close();
<? } >

var macacl = {
	init: function() {
		var tbody = $('#macacl_table > tbody')[0],
		    errDiv = $("#macaclErrDiv"),
		    that = this;

		$('#macacl_table')
			.delegate('input.enableMacacl', 'change', function() {
				var macaclObj = $.tmplItem(this).data,
				    enabled = this.checked;
				macaclObj.enabled(enabled);
			})
			.delegate('input.editMacacl', 'click', function() {
				var macaclObj = $.tmplItem(this).data;
				$(this).closest('tr').replaceWith($('#macaclEditTmpl').tmpl([macaclObj]));
			})
			.delegate('input.saveMacacl', 'click', function() {
				var macaclObj = $.tmplItem(this).data;
				macaclObj.comment = $('#edtMacaclComment').val();
				$(this).closest('tr').replaceWith($('#macaclTmpl').tmpl([macaclObj]));
			})
			.delegate('input.delMacacl', 'click', function() {
				var macacl = g_cfg.get('wireless.1.mac_acl'),
				    macaclObj = $.tmplItem(this).data;
				macacl.remove(macaclObj);
				$(this).closest('tr').remove();
			});

		$('#macaclAdd').click(function() {
			// TODO: add validation
			var macacl = g_cfg.get('wireless.1.mac_acl'),
			    macaclObj = macacl.create(
				$('#macaclMac').val().toUpperCase(),
				$('#macaclComment').val(),
				true),
			    errors = [];
			if (macaclObj.valid(errors)) {
				if (macacl.find({ mac: macaclObj.mac }).length) {
					errors.push('Acl already exist.');
				}
				else {
					var macacl = g_cfg.get('wireless.1.mac_acl');
					macacl.add(macaclObj);
					$('#macacl_table tfoot input[type=text]').val('');
					$('#macaclTmpl')
						.tmpl(macaclObj)
						.appendTo('#macacl_table');
				}
			}
			showErrors(errDiv, errors);
		});
	},
	render: function() {
		var macacl = g_cfg.get('wireless.1.mac_acl');
		$('#macacl_table > tbody > tr').remove();
		$('#macaclTmpl')
			.tmpl(macacl.objs)
			.appendTo('#macacl_table');
	}
};

function showErrors(div, errors) {
	var rows = [];
	if (errors && errors.length > 0) {
		for (var i = 0; i < errors.length; ++i) {
			if (rows.length > 0)
				rows.push('<br/>');
			if (errors[i].length > 0)
				rows.push(errors[i]);
		}
	}
	div.html(rows.join('')).toggle(rows.length > 0);
}

$(document).ready(function(){

	// Section toggling
	$("h2.trigger").click(function(){
		var collapsed = "";
		$(this).toggleClass("active").next().slideToggle("fast");
		$('.active').each(function() {
			collapsed = collapsed + (collapsed ? ',#' : '#') + this.id;
		});
		$.cookie('netCollapsed', collapsed);
		return false;
	});

	$.ajax({
		url : 'getcfg.sh',
		data : '.',
		type : 'GET',
		dataType : 'text',

		success : function(rxcfg) {
			g_cfg = ubnt.cfg.parse(rxcfg);
			macacl.init();
			macacl.render();
		},
		error : function(xhr, status) {
			// TODO: handle error while receiving configuration
			alert('Failed!');
		}
	});
        
	$('#macacl').submit(function() {
		try {
			$('#macacl_data').val(ubnt.cfg.toCfg(g_cfg).join('\r\n'));
			return true;
		}
		catch (e) {
			alert('An error has occurred: ' + e.message);
		}
		return false;
	});
});
//-->
</script>
</head>
<body class="popup">
	<form id="macacl" name="macacl" enctype="multipart/form-data" action="<? echo $PHP_SELF;>" method="POST">
		<table cellspacing="0" cellpadding="0" align="center" class="popup">
			<tr>
				<th colspan="3"><? echo dict_translate("MAC ACL"); ></th>
			</tr>
			<tr>
				<td colspan="3">
					<div id="macaclErrDiv" class="ui-state-error ui-corner-all initial_hide errors">
					</div>
					<table id="macacl_table" class="settings" style="width: 500px">
						<thead>
							<tr>
								<th class="enabled"><? echo dict_translate("Enabled"); ></th>
								<th class="ip"><? echo dict_translate("MAC"); ></th>
								<th class="comment"><? echo dict_translate("Comment");></th>
								<th class="action"><? echo dict_translate("Action"); ></th>
							</tr>
						</thead>
						<tfoot>
							<tr>
								<td></td>
								<td><input type="text" id="macaclMac" /></td>
								<td><input type="text" id="macaclComment" /></td>
								<td>
									<input type="button" id="macaclAdd" value="<? echo dict_translate("Add")>" />
								</td>
							</tr>
						</tfoot>
						<tbody><tbody>
					</table>
				</td>
			</tr>
			<tr>
				<td colspan="3" class="centered">
					<input type="hidden" name="macacl_data" id="macacl_data" value="">
					<input type="submit" name="macacl_submit" value="<? echo dict_translate("Save")>">
					<input type="button" name="cancel" value="<? echo dict_translate("Cancel")>" onClick="window.close()">
				</td>
			</tr>
		</table>
	</form>
</body>
</html>
