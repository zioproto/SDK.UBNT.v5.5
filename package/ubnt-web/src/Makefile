.PHONY: all clean install install-local

instdir := ${build_install_directory}/usr/www
ACCESS4ALL = \
arp.cgi \
brmacs.cgi \
dhcpc.cgi \
dhcpcinfo.cgi \
fw.cgi \
glogo.cgi \
index.cgi \
info.cgi \
jsl10n.cgi \
leases.cgi \
log.cgi \
login.cgi \
logo.cgi \
logout.cgi \
pfw.cgi \
pingtest.cgi \
pingtest_action.cgi \
poll.cgi \
pppinfo.cgi \
signal_gui.cgi \
sptest.cgi \
sptest_action.cgi \
sroutes.cgi \
stainfo.cgi \
stalist.cgi \
sta.cgi \
throughput.cgi \
traceroute.cgi \
traceroute_action.cgi \
update_check.cgi

DENY4DEMO = admin.cgi \
            apply.cgi \
	    discard.cgi \
	    fwflash.cgi \
	    reboot.cgi \
	    reset.cgi \
	    test.cgi \
	    testdone.cgi \
	    discovery.cgi \
	    upgrade.cgi \
	    dismiss.cgi

all:

clean:
	find -name "*~" -exec rm -fr {} \;

install:
	mkdir -p ${instdir}
	find ${instdir} -type l -exec rm -f {} \;
	./dict.sh > English.txt
	(tar -X .excluded -cf - .) | (cd ${instdir} && tar xvf -)
	rm -f $(instdir)/images/logo.gif && ln -sf /etc/persistent/www/logo.gif $(instdir)/images/logo.gif
	( \
		current_build_version=${FULL_VERSION}; \
		if [ "." = ".$$current_build_version" ]; then \
			current_build_version=`cat ${build_install_directory}/usr/lib/version`; \
		fi; \
		find $(instdir) -type f | xargs \
		sed -i -r "s%FULL_VERSION_LINK%/$$current_build_version%g;s%FULL_VERSION_NUMBER%$$current_build_version%g"; \
		cd $(instdir) && ln -sf . $$current_build_version \
	)
	find $(instdir) -type f -a -name '*.js' -a -exec  yui-compressor --type js -o {} {} \;
	find $(instdir) -type f -a \( -name '*.js' -o -name '*.css' \) -a -size +2k -a -exec sh -c "gzip -9 -c {} > {}.gz" \;
	cd $(instdir); chmod 750 *.cgi; chmod 755 $(ACCESS4ALL); chmod 700 $(DENY4DEMO)

### CUT HERE ###
LOCAL_DIR = ~/chroot/lsx/usr/www
install-local:
	./dict.sh > English.txt
	(tar -X .excluded -cf - .) | (cd ${LOCAL_DIR} && tar xvf -)
	cd ${LOCAL_DIR} && rm -f FULL_VERSION_LINK && ln -sf . FULL_VERSION_LINK
