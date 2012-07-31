function getXmlHttpObject()
{
	var xmlHttp=null;
	try {
		// Firefox, Opera 8.0+, Safari
		xmlHttp=new XMLHttpRequest();
	} catch (e) { // Internet Explorer
    	try	{
			xmlHttp=new ActiveXObject("Msxml2.XMLHTTP");
		} catch (e) {
			xmlHttp=new ActiveXObject("Microsoft.XMLHTTP");
		}
	}
	return xmlHttp;
}

var AjaxConstants = 
{ 
	NULL_FUNC:function() {} 
} 

function ajax()
{
	this.req = null;

	this.abort = function()
	{
		if (!this.req)
		{
			return;
		}
		this.req.onreadystatechange = AjaxConstants.NULL_FUNC;
		try /* in case if there HmlHttp has no abort() method. */
		{
			this.req.abort();
		}
		catch (e)
		{
		}
		this.req = null;
	}

	this.toString = function() { return "ajax"; };
	this.sendRequest = function(method, url, callback) {
		var req = getXmlHttpObject();
		if (req == null) {
			return false;
		}
		try {
			req.open(method, url, true);
		}
		catch (e)
		{
			return false;
		}
		req.onreadystatechange = function()
		{
			if (callback == null)
			{
				req.onreadystatechange = function() {};
				return;
			}
			if (req.readyState == 4) {
				req.onreadystatechange = function() {};
				callback(req);
				delete req;
			}
		};
		req.setRequestHeader("X-Requested-With", "XMLHttpRequest");
		req.send(null);
		this.req = req;
		return true;
	};
	this.get = function(url, callback) {
		return this.sendRequest('GET', url, callback);
	};
}
