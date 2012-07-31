/*
 * Copyright 2006-2012, Ubiquiti Networks, Inc. <gpl@ubnt.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of other contributors
 *	  may be used to endorse or promote products derived from this software
 *	  without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <ber/ber.h>
#include <debug/memory.h>
#include <debug/log.h>
#include <abz/typedefs.h>
#include <abz/error.h>

#include <tinysnmp/tinysnmp.h>
#include <tinysnmp/agent/module.h>
#include <tinysnmp/agent/odb.h>
#include <tinysnmp/agent/ifcache.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>		/* For AF_INET & struct sockaddr */
#include <linux/wireless.h>

#ifndef __packed
#define	__packed	__attribute__((__packed__))
#endif

#define	STAINFO_BUF_SIZE	256 * IFHWADDRLEN /* should be enough for everyone */

#define	IEEE80211_IOCTL_STA_STATS	(SIOCDEVPRIVATE+5)
#define	IEEE80211_IOCTL_GETMACS		(SIOCDEVPRIVATE+12)
#define	IEEE80211_IOCTL_STA_INFO2	(SIOCDEVPRIVATE+13)


/*
 To sync code use the latest: 80211stats -d all
 to get more information about field offsets shift
 */
#define IEEE80211_NODESTATS_SIZE 216
struct ieee80211req_sta_stats {
	union {
		u_int8_t	macaddr[6];
		u_int64_t	pad;
	} is_u;
	unsigned char data[IEEE80211_NODESTATS_SIZE];
};
#define ns_rx_data(x) (*(u_int32_t*)&((x)[0]))
#define ns_rx_bytes(x) (*(u_int64_t*)&((x)[24]))
#define ns_tx_data(x) (*(u_int32_t*)&((x)[96]))
#define ns_tx_bytes(x) (*(u_int64_t*)&((x)[112]))

#define IEEE80211REQ_STA_INFO_SIZE 896
#define isi_macaddr(x) ((u_int8_t*)&((x)[26]))
#define isi_txratekbps(x) (*(u_int32_t*)&((x)[764]))
#define isi_rxratekbps(x) (*(u_int32_t*)&((x)[768]))
#define isi_noisefloor(x) (*(u_int16_t*)&((x)[774]))
#define isi_signal(x) (*(int8_t*)&((x)[888]))

static int
update(struct odb **odb, const uint32_t *oid, uint8_t type, void *data)
{
	snmp_value_t value;
	value.type = type;

	switch (type) {
		case BER_INTEGER:
			value.data.INTEGER = *(int32_t *) data;
			break;
		case BER_Counter32:
			value.data.Counter32 = *(uint32_t *) data;
			break;
		case BER_Gauge32:
			value.data.Gauge32 = *(uint32_t *) data;
			break;
		case BER_TimeTicks:
			value.data.TimeTicks = *(uint32_t *) data;
			break;
		case BER_Counter64:
			value.data.Counter64 = *(uint64_t *) data;
			break;
		case BER_OID:
			value.data.OID = (uint32_t *) data;
			break;
		case BER_OCTET_STRING:
			value.data.OCTET_STRING = *(octet_string_t *) data;
			break;
		default:
			abz_set_error ("invalid type (0x%02x) specified",type);
			return (-1);
	}

	return (odb_add(odb, oid, &value));
}

/*
 * Open a socket (taken from iwlib.c)
 * Depending on the protocol present, open the right socket. The socket
 * will allow us to talk to the driver.
 */
static int
open_socket(void)
{
	static const int families[] = {
		AF_INET, AF_IPX, AF_AX25, AF_APPLETALK
	};
	unsigned int i;
	int sock;

	/*
	 * Now pick any (exisiting) useful socket family for generic queries
	 * Note : don't open all the socket, only returns when one matches,
	 * all protocols might not be valid.
	 * Workaround by Jim Kaba <jkaba@sarnoff.com>
	 * Note : in 99% of the case, we will just open the inet_sock.
	 * The remaining 1% case are not fully correct...
	 */

	/* Try all families we support */
	for(i = 0; i < sizeof(families)/sizeof(int); ++i) {
		/* Try to open the socket, if success returns it */
		sock = socket(families[i], SOCK_DGRAM, 0);
		if(sock >= 0)
			return sock;
	}

	return -1;
}

static void
mtik_wireless_station_update(struct odb **odb, int skfd, char *ifname, int ifidx)
{
	struct iwreq	iwr;                       // ioctl request structure
	uint8_t			bfr[34];
	struct iw_statistics	iwstats;
	uint32_t 		oid[14] = {13, 43, 6, 1, 4, 1, 14988, 1, 1, 1, 1, 1, 1, 0};
	octet_string_t	str;

	int len;
	u_int8_t *buf;

	oid[13] = ifidx;
	strncpy(iwr.ifr_name, ifname, IFNAMSIZ);

	// At first check if am working in STA mode
	if ((ioctl(skfd, SIOCGIWMODE, &iwr) < 0) || (iwr.u.mode != IW_MODE_INFRA))
		return;

	// Get ESSID
	iwr.u.essid.pointer = (caddr_t) &bfr;
	memset(&bfr, 0, sizeof(bfr));
	iwr.u.essid.length = IW_ESSID_MAX_SIZE + 1;
	iwr.u.essid.flags = 0;
	if (ioctl(skfd, SIOCGIWESSID, &iwr) >= 0 ) {
		oid[12] = 5;
		str.len = iwr.u.essid.length;
		str.buf = bfr;
		update(odb, oid, BER_OCTET_STRING, &str);
	}

	// Get frequency/channel
	if ( ioctl ( skfd, SIOCGIWFREQ, &iwr ) >= 0 ) {
		uint32_t freq = iwr.u.freq.m;
		while (freq > 9999)
			freq /= 10;
		oid[12] = 7;
		update(odb, oid, BER_INTEGER, &freq);
	}

	// Get BSSID address
	if ( ioctl ( skfd, SIOCGIWAP, &iwr ) >= 0 ) {
		int found = 0;
		oid[12] = 6;
		str.len = IFHWADDRLEN;
		str.buf = bfr;
		memcpy(bfr, iwr.u.ap_addr.sa_data, IFHWADDRLEN);
		update(odb, oid, BER_OCTET_STRING, &str);

		// I'll try to get all the rest info via single ioctl
		buf = malloc(STAINFO_BUF_SIZE);
		iwr.u.data.pointer = (void*)buf;
		iwr.u.data.length = STAINFO_BUF_SIZE;
		memcpy(isi_macaddr(buf), bfr, IFHWADDRLEN);
		if (ioctl(skfd, IEEE80211_IOCTL_STA_INFO2, &iwr) >= 0) {
			len = iwr.u.data.length;
			if (len >= IEEE80211REQ_STA_INFO_SIZE) {
				int tmp = isi_signal(buf);
				oid[12] = 4;
				update(odb, oid, BER_INTEGER, &tmp);

				oid[12] = 2;
				tmp = isi_txratekbps(buf) * 1000;
				update(odb, oid, BER_Gauge32, &tmp);

				oid[12] = 3;
				tmp = isi_rxratekbps(buf) * 1000;
				update(odb, oid, BER_Gauge32, &tmp);

				found = 1;
			}
		}
		free(buf);
		if (found)
			return;
	}

	// failed to get via Atheros IOCTL. Try continue in traditional way
	// Get TX Rate
	if ( ioctl ( skfd, SIOCGIWRATE, &iwr ) >= 0 ) {
		oid[12] = 2;
		update(odb, oid, BER_Gauge32, &iwr.u.bitrate.value);
	}

	// get signal strength
	iwr.u.data.pointer = ( caddr_t ) &iwstats;
	iwr.u.data.length = 0;
	iwr.u.data.flags = 1;   // Clear updated flag
	if ( ioctl ( skfd, SIOCGIWSTATS, &iwr ) >= 0 ) {
		int strength = iwstats.qual.level - 0x100;
		oid[12] = 4;
		update(odb, oid, BER_INTEGER, &strength);
	}
}

static void
mtik_node_update(struct odb **odb, int skfd, char *ifname,
		int ifidx, unsigned char* buf)
{
	struct iwreq iwr;
	struct ieee80211req_sta_stats stats;
	int tmp;

	uint32_t oid[20] = { 19, 43, 6, 1 ,4, 1, 14988, 1, 1, 1, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0 };

	oid[13] = isi_macaddr(buf)[0];
	oid[14] = isi_macaddr(buf)[1];
	oid[15] = isi_macaddr(buf)[2];
	oid[16] = isi_macaddr(buf)[3];
	oid[17] = isi_macaddr(buf)[4];
	oid[18] = isi_macaddr(buf)[5];

	oid[19] = ifidx;

	oid[12] = 3;
	tmp = isi_signal(buf);
	update(odb, oid, BER_INTEGER, &tmp);

	memset(&iwr, 0, sizeof(iwr));
	strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
	iwr.u.data.pointer = (void *) &stats;
	iwr.u.data.length = sizeof(stats);
	memcpy(stats.is_u.macaddr, isi_macaddr(buf), sizeof(stats.is_u.macaddr));
	if (ioctl(skfd, IEEE80211_IOCTL_STA_STATS, &iwr) < 0) {
		log_printf(LOG_WARNING,
				"unable to get node information on %s\n", ifname);
		return;
	}

	oid[12] = 4;
	tmp = ns_tx_bytes(stats.data);
	update(odb, oid, BER_Counter32, &tmp);
	oid[12] = 5;
	tmp = ns_rx_bytes(stats.data);
	update(odb, oid, BER_Counter32, &tmp);
	oid[12] = 6;
	tmp = ns_tx_data(stats.data);
	update(odb, oid, BER_Counter32, &tmp);
	oid[12] = 7;
	tmp = ns_rx_data(stats.data);
	update(odb, oid, BER_Counter32, &tmp);

	oid[12] = 8;
	tmp = isi_txratekbps(buf) * 1000;
	update(odb, oid, BER_Gauge32, &tmp);
	oid[12] = 9;
	tmp = isi_rxratekbps(buf) * 1000;
	update(odb, oid, BER_Gauge32, &tmp);
}

static unsigned char*
get_sta_info2(int sock, const char* ifname, const char* mac) {
	struct iwreq iwr;
	unsigned char* buf;
	int res, len = 2048;

	buf = calloc(1, len);
	if (!buf)
		return NULL;

	memset(&iwr, 0, sizeof(iwr));
	strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
	do {
		memcpy(isi_macaddr(buf), mac, IFHWADDRLEN);
		iwr.u.data.pointer = (void*) buf;
		iwr.u.data.length = len;

		res = ioctl(sock, IEEE80211_IOCTL_STA_INFO2, &iwr);
		if (res != 0 && errno == E2BIG)
		{
			uint8_t* p;

			len = len * 2 < USHRT_MAX ? len * 2 : USHRT_MAX;
			p = (uint8_t*)realloc(buf, len);
			if (!p) {
				free(buf);
				return NULL;
			}

			buf = p;
		}
	} while (res != 0 && errno == E2BIG && len < USHRT_MAX);

	if (res < 0) {
		free(buf);
		return NULL;
	}

	return buf;
}

static int
mtik_registration_table_update(struct odb **odb, int skfd, char *ifname, int ifidx)
{
	int i;
	struct iwreq iwr;
	u_int8_t *si;
	u_int8_t *buf = malloc(STAINFO_BUF_SIZE);

	memset(&iwr, 0, sizeof(iwr));
	strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
	iwr.u.data.pointer = (void*)buf;
	iwr.u.data.length = STAINFO_BUF_SIZE;

	if (ioctl(skfd, IEEE80211_IOCTL_GETMACS, &iwr) < 0) {
		log_printf(LOG_WARNING, "unable to get station information on %s\n", ifname);
		free(buf);
		return -1;
	}

	for (i = 0; i < iwr.u.data.length / IFHWADDRLEN; i++) {
		char* mac = iwr.u.data.pointer + i * IFHWADDRLEN;
		si = get_sta_info2(skfd, ifname, mac);
		if (!si)
			continue;
		mtik_node_update(odb, skfd, ifname, ifidx, si);
		free(si);
	}

	free(buf);
	return 0;
}

static int
mtik_wireless_update (struct odb **odb)
{
	struct iwreq iwr;                       // ioctl request structure
	FILE *fp;
	char  bfr[1024], ifName[IFNAMSIZ+1];
	char *s, *t;
	int ifIndex = 0;
	int skfd = open_socket();

	if (skfd < 0) {
		log_printf (LOG_ERROR, "SNMP mikrotik.%s(): socket open failure\n", __func__ );
		return -1;
	}

	odb_destroy (odb);

	// find interfaces in /proc/net/dev and find the wireless interfaces
	fp = fopen("/proc/net/dev", "r");
	if (fp) {
		while (fgets (bfr, sizeof (bfr), fp)) {
			if (strstr(bfr, ":")) {
				s = bfr; t = ifName;
				while (isspace (*s)) // discard white space
					*s++;
				while (*s != ':')     // get interface name
					*t++ = *s++;
				*t = '\0';

				ifIndex = ifcache_get_ifindex(ifName);
				if (!ifIndex) {
					log_printf (LOG_WARNING, "SNMP mikrotik.%s()  `%s` has no ifIndex\n",
							__func__, ifName);
					continue;
				}

				// verify as a wireless device
				strncpy(iwr.ifr_name, ifName, IFNAMSIZ);
				if ( ioctl ( skfd, SIOCGIWNAME, &iwr ) >= 0 ) {
					mtik_wireless_station_update(odb, skfd, ifName, ifIndex);
					mtik_registration_table_update(odb, skfd, ifName, ifIndex);
				}
			}
		}
		fclose(fp);
	}

	close(skfd);
	return 0;
}

static const uint32_t mtik_oid[] = { 6, 43, 6, 1, 4, 1, 14988 };

struct module module =
{
	.name	= "mikrotik",
	.descr	= "The Mikrotik experimental wireless MIB module ",
	.mod_oid	= mtik_oid,
	.con_oid	= mtik_oid,
	.parse	= NULL,
	.open	= NULL,
	.update	= mtik_wireless_update,
	.close	= NULL
};
