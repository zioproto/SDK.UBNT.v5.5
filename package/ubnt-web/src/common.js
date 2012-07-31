function devname2uidevname(devname) {
	var res = devname.replace("ath","WLAN");
        res = res.replace("eth","LAN");
        res = res.replace("br","BRIDGE");
        res = res.replace("ppp","TUNNEL");
        return res;
}

function uidevname2devname(uidevname) {
	var res = uidevname.replace("WLAN","ath");
        res = res.replace("LAN","eth");
        res = res.replace("BRIDGE","br");
        res = res.replace("TUNEL","ppp");
        return res;
}

function add_ifc_options(e, s, ifcs, first) {
	var html = "";
        if (!(typeof first == "undefined")) {
	       	html += "<option value='0'>";
       		html += first+"</option>";
        }
        for(var i = 0; i < ifcs.length; ++i) {
		html += "<option value='"+ifcs[i]+"'>";
		html += devname2uidevname(ifcs[i])+"</option>";
        }
        e.html(html);
        e.val(s);
}

function get_br_ports(dev, br_cfg, all) {
	var ret = [];
	for(var i = 0; i < br_cfg.objs.length; i++) {
        	if (br_cfg.objs[i].devname != dev) continue;
        	var ports = br_cfg.objs[i].port;
                for(var j = 0; j < ports.length; j++) {
                	if ((!ports[j].enabled && !all) || ($.inArray(ports[j].devname, ret) != -1))
                        	continue;
                        ret.push(ports[j].devname);
                }
        }
        return ret;
}

function get_bridge(port, br_cfg, any) {
        if (typeof any == "undefined") any = false;
	for(var i = 0; i < br_cfg.objs.length; i++) {
        	var br = br_cfg.objs[i];
                if (!br.enabled) continue;
        	for(var j = 0; j < br.port.length; j++) {
                	if ((br.port[j].devname == port) &&
                            (br.port[j].enabled || all))
                        	return br.devname;
                }
        }
        return "";
}

function get_all_br_ports(br_cfg) {
	var ret = [];
        if (!br_cfg.enabled) return ret;
        
	for(var i = 0; i < br_cfg.objs.length; i++) {
        	if (!br_cfg.objs[i].enabled) continue;
        	var ports = br_cfg.objs[i].port;
                for(var j = 0; j < ports.length; j++) {
                	if (!ports[j].enabled || ($.inArray(ports[j].devname, ret) != -1))
                        	continue;
                        ret.push(ports[j].devname);
                }
        }
        return ret;
}

function get_br_disabled_port(dev, br_cfg) {
	for(var i = 0; i < br_cfg.objs.length; i++) {
        	if (br_cfg.objs[i].devname != dev) continue;
        	var ports = br_cfg.objs[i].port;
                for(var j = 0; j < ports.length; j++) {
                	if (!ports[j].enabled)
                        return ports[j].devname;
                }
        }
        return "";
}

function get_disabled_ifc(net_cfg, ifc_cfg) {
	for(var i = 0; i < ifc_cfg.objs.length; i++) {
                for(var j = 0; j < net_cfg.objs.length; j++) {
                       	if (net_cfg.objs[j].devname != ifc_cfg.objs[i].devname) continue;
                	if (!net_cfg.objs[j].enabled)
                        	return net_cfg.objs[j].devname;
                }
        }
        return "";
}

function prepare_vlan_ifc(vlan_cfg, exclude, all) {
	var vlan_ifcs = [];
        var obj, i;
        var c = vlan_cfg.objs.length;
        if (!$.isArray(exclude)) exclude = [];
        if (typeof all == "undefined") all = false;
        if (vlan_cfg.enabled || all)
		for (i = 0; i < c; i++){
        		obj = vlan_cfg.objs[i];
                        var vlan_dev = obj.devname+"."+obj.id;
        		if (!(obj.enabled || all) ||
        		    ($.inArray(vlan_dev, exclude)!=-1) ||
                            ($.inArray(vlan_dev, vlan_ifcs)!=-1)) continue;
	                vlan_ifcs[vlan_ifcs.length] = vlan_dev;
        	}
	return vlan_ifcs;
}

function remove_ifc(dev, net_cfg) {
       	for (i = 0; i < net_cfg.objs.length; i++){
       		obj = net_cfg.objs[i];
       		if (obj.devname != dev) continue;
                net_cfg.objs.splice(i, 1);
       	}
        return net_cfg;
}

function prepare_ifc(net_cfg, exclude, all) {
	var net_ifcs = [];
        var obj, i;
        var c = net_cfg.objs.length;
        if (!$.isArray(exclude)) exclude = [];
        if (typeof all == "undefined") all = false;
        if (net_cfg.enabled || all)
		for (i = 0; i < c; i++){
        		obj = net_cfg.objs[i];
        		if (!(obj.enabled || all) ||
			    ($.inArray(obj.devname, exclude)!=-1) ||
                            ($.inArray(obj.devname, net_ifcs)!=-1)) continue;
	                net_ifcs[net_ifcs.length] = obj.devname;
        	}
	return net_ifcs;
}

function common_on_close(e) {
	window.close();
}

function common_on_save(frm) {
	if (!validateStandard($(frm)[0], 'error'))
        	return false;
	var res = "";
        if (typeof prepare_cfg == "function")
        	res = prepare_cfg();//prepare_json()
        $(frm+"_data").val(res);
        $(frm).submit();
        /*
        if (window.opener && !window.opener.closed) {
        	if (window.opener.doSubmit)
                	window.opener.doSubmit();
        	else
                	window.opener.location.href=window.opener.location.href;
        }
        */
        return true;
}

function common_on_add(e) {
	if (typeof add_data == "function")
		add_data(null);
}

function common_recount(e) {
        var nos = $('[ui_id="no"]');
        for(var i = 0; i < nos.length; i++) {
        	nos.eq(i).text((i+1)+".");
        }
}

function common_on_del(e) {
        var obj = $(this);
        if (!obj.is(':visible')) return;
        obj.parent().parent().remove();
        obj = $("tr.row").last();
        $('[ui_id="add"]', obj).removeClass("initial_hide");
        $('[ui_id="del"]', obj).addClass('initial_hide');
        common_recount();
}

function common_on_del_all(e) {
	var obj = $('[ui_id="del"]');
        obj.each(common_on_del);
}

function common_on_change(e) {
        var row = $(this).parent().parent();
        var req = $('[ui_id="status"]', row).attr('checked');
        if (typeof on_change == "function")
        	on_change(row, req);
}

function common_init() {
	if (typeof on_save == "function")
	        $("#save").bind("click", on_save);
        $("#close").bind("click", common_on_close);
        $("#del_all").bind("click", common_on_del_all);
        $('[ui_id="del"]').bind("click", common_on_del);
        $('[ui_id="add"]').bind("click", common_on_add);
        $('[ui_id="status"]').bind("change", common_on_change);
        if (typeof on_init == "function")
        	on_init();
}

$(document).ready(common_init);
