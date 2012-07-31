$(function() {
	$('a[rel=help]').click(function() {
		var pos = $(this).position();
		var x = pos.left + $(this).outerWidth();
		var y = pos.top - $(document).scrollTop();

		var data = $('<div/>').load($(this).attr("href"));
		data.attr('title', 'Help');
		data.dialog(
		{
			bgiframe: true,
			modal : true,
			height : 'auto',
			width: 300,
			resizable : false,
			position : [x, y],
			buttons: {
				"Close": function() { $(this).dialog("close"); }
			}
		});
		return false;
	});
});
