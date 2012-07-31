/* Do Protocol Translation IPv4->IPv6
 * IVI Project, CERNET
 * Neil 2005.11.02
 *
 * add:   Path MTU Discovery 2005.11.11  Neil
 * patch: "TCP in ICMP" translation error 2006.4.19 Neil
 */

#include <linux/in6.h>
#include <linux/in.h>
#include <linux/ipv6.h>
#include <net/ipv6.h>	/* for frag_hdr */
#include <linux/ip.h>
#include <linux/icmp.h>
#include <linux/icmpv6.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <asm/atomic.h>
#include <asm/checksum.h>
#include <linux/mapping_config.h>
#include "proto_trans.h"


void mapping_address4to6(struct in6_addr *newv6, const __u32 oldv4, const __u32 bbprefix, const __u32 fragmentation) {
unsigned char * startpos;
#if defined(__BIG_ENDIAN_BITFIELD)
        newv6->s6_addr32[3] = bbprefix;
        newv6->s6_addr32[2] = oldv4;
        newv6->s6_addr32[1] = fragmentation;
        newv6->s6_addr32[0] = oldv4;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
        newv6->s6_addr32[0] = bbprefix;
	newv6->s6_addr32[1] = 0;
	newv6->s6_addr32[2] = 0;
	newv6->s6_addr32[3] = 0;
	startpos = (unsigned char *)(newv6->s6_addr32 + 1);
	*startpos = 0xFF;
	*((__u32 *)(startpos + 1)) = oldv4;
#else
#error  "Please fix <asm/byteorder.h>"
#endif

#ifdef	MAPPING_ADDRESS_CHECKSUM
	newv6->s6_addr32[2] = 0;
	newv6->s6_addr32[2] = ip_compute_csum((unsigned char *)(newv6->s6_addr32), 16);
#endif

}

unsigned short iphdr4to6_trans(unsigned char *newdata, struct iphdr *oldhdr, __u32 src_prefix, __u32 dst_prefix, int supplement, int fragment) {
	struct frag_hdr *ext_header;
	struct ipv6hdr *newhdr = (struct ipv6hdr *)newdata;
	*(__u32*)newhdr = htonl(0x60000000);
	newhdr->nexthdr = oldhdr->protocol;
#ifdef 	MAPPING_ICMP
	if (oldhdr->protocol == IPPROTO_ICMP) 
		newhdr->nexthdr = IPPROTO_ICMPV6;
#endif
        newhdr->hop_limit = oldhdr->ttl;
       	newhdr->payload_len = htons(ntohs(oldhdr->tot_len) - IP_HEADER_LEN + supplement);	 

#ifdef	MAPPING_FRAGMENTATION
        mapping_address4to6(&(newhdr->saddr), oldhdr->saddr, src_prefix, *((__u32 *)(&(oldhdr->id))));
#else
	mapping_address4to6(&(newhdr->saddr), oldhdr->saddr, src_prefix, 0);
#endif
        mapping_address4to6(&(newhdr->daddr), oldhdr->daddr, dst_prefix, 0);
#ifdef 	MAPPING_EXT_HEADER
	if (fragment) {
		ext_header = (struct frag_hdr *)(newdata + IPV6_HEADER_LEN);
		ext_header->nexthdr = newhdr->nexthdr;
		newhdr->nexthdr = NEXTHDR_FRAGMENT;
		ext_header->identification = oldhdr->id;
		ext_header->frag_off = oldhdr->frag_off;
		return ext_header->nexthdr;
	}
	else return newhdr->nexthdr;
#else
	return newhdr->nexthdr;
#endif
}

/*	Get new IPV6 header length	*/
unsigned get_new_headerlen4to6( struct iphdr *oldhdr ) {
	if ((ntohs(oldhdr->frag_off) & 0x4000) == 0) return IPV6_HEADER_LEN + 8;
	else if ((ntohs(oldhdr->frag_off) & 0x2000) != 0) return IPV6_HEADER_LEN + 8;
	else return IPV6_HEADER_LEN;
}

/*	Get new IPV6 packet payload length	*/
unsigned get_new_pktlen4to6( struct sk_buff *skb )
{
	struct iphdr * hdr = (struct iphdr *)ip_hdr(skb);
	struct icmphdr * ihdr = (struct icmphdr *)skb->data;
	
	if ( hdr->protocol != IPPROTO_ICMP )
		return skb->len;
	else if ( (ntohs(hdr->frag_off) & 0x1FFF) != 0 ) /* if this is not the first fragment, we *SHOULD NOT* read the app-layer header */
		return skb->len;
	else if ( ihdr->type == ICMP_ECHO || ihdr->type == ICMP_ECHOREPLY )
		return skb->len;
	else {
		short int header_in_pkt = get_new_headerlen4to6((struct iphdr *)(skb->data + ICMP_HEADER_LEN));
		return skb->len + header_in_pkt - IP_HEADER_LEN; /*IPv6 in ICMP data*/
	}
}



/*Translate ICMP to ICMPv6*/

int icmp4to6_trans(unsigned char * new, unsigned char * old, unsigned new_len, unsigned old_len, __u32 src_prefix, __u32 dst_prefix, struct in6_addr * src6, struct in6_addr * dst6)
{
	struct icmphdr * oldhdr = (struct icmphdr *) old;
	struct icmp6hdr * newhdr = (struct icmp6hdr *) new;
	unsigned short newproto = NO_PROTO;
	unsigned ready_len = 0;
	unsigned pass_len = 0;
	unsigned header_in_pkt = 0;

	
	if(old_len<ICMP_HEADER_LEN)
		return 1;
	
	switch(oldhdr->type)
	{
		case ICMP_ECHOREPLY:
			newhdr->icmp6_type=ICMPV6_ECHO_REPLY;
			ready_len += ICMP_HEADER_LEN;
			pass_len += ICMP_HEADER_LEN;
			break;
		case ICMP_ECHO:
			newhdr->icmp6_type=ICMPV6_ECHO_REQUEST;
			ready_len += ICMP_HEADER_LEN;
			pass_len += ICMP_HEADER_LEN;
			break;
		case ICMP_DEST_UNREACH:
			if ( oldhdr->code != ICMP_FRAG_NEEDED )
			{
				newhdr->icmp6_type=ICMPV6_DEST_UNREACH;
				if (oldhdr->code == 3) oldhdr->code = 4;
			}
			else
			{
				newhdr->icmp6_type=ICMPV6_PKT_TOOBIG;
				struct iphdr *hh = (struct iphdr *)(old + ICMP_HEADER_LEN);
				if ((hh->frag_off & 0x4000) == 0 || (hh->frag_off & 0x2000) != 0)
					oldhdr->un.gateway = htonl(ntohl(oldhdr->un.frag.mtu)+28);/*note*/
				else
					oldhdr->un.gateway = htonl(ntohl(oldhdr->un.frag.mtu)+20);
			}
			if (old_len>=IP_ICMP_LEN)
			{
				header_in_pkt = get_new_headerlen4to6((struct iphdr*)(old + ICMP_HEADER_LEN));
				newproto=iphdr4to6_trans((new + ICMP_HEADER_LEN),(struct iphdr *)(old + ICMP_HEADER_LEN), src_prefix, dst_prefix, header_in_pkt - IPV6_HEADER_LEN, header_in_pkt - IPV6_HEADER_LEN);
				ready_len += (8 + header_in_pkt);
				pass_len += IP_ICMP_LEN;
			}
			else
			{
				ready_len += ICMP_HEADER_LEN;
				pass_len += ICMP_HEADER_LEN;
			}
			break;
		case ICMP_TIME_EXCEEDED:
			newhdr->icmp6_type=ICMPV6_TIME_EXCEED;
			if (old_len>=IP_ICMP_LEN)
			{
				header_in_pkt = get_new_headerlen4to6((struct iphdr*)(old + ICMP_HEADER_LEN));
				newproto=iphdr4to6_trans((new + ICMP_HEADER_LEN),(struct iphdr *)(old + ICMP_HEADER_LEN), src_prefix, dst_prefix, header_in_pkt - IPV6_HEADER_LEN, header_in_pkt - IPV6_HEADER_LEN);
				ready_len += (8 + header_in_pkt);
				pass_len += IP_ICMP_LEN;
			}
			else
			{
				ready_len += ICMP_HEADER_LEN;
				pass_len += ICMP_HEADER_LEN;
			}
			break;
		default:break;
	}
	newhdr->icmp6_code = oldhdr->code;
	newhdr->icmp6_dataun.un_data32[0] = oldhdr->un.gateway;
	switch(newproto)
	{
		case NO_PROTO:
			break;
		case IPPROTO_ICMPV6:
			if(!icmp4to6_trans(new+ready_len,old+pass_len,new_len - ready_len, old_len-pass_len, src_prefix, dst_prefix, (struct in6_addr *)(new+ICMP_HEADER_LEN+IP6_SADDR), (struct in6_addr *)(new+ICMP_HEADER_LEN+IP6_DADDR)))
			{
				ready_len += ICMP_HEADER_LEN;
				pass_len += ICMP_HEADER_LEN;
			}
			break;
		case IPPROTO_TCP:
			if(!tcp4to6_trans(new+ready_len,old+pass_len,new_len - ready_len, old_len-pass_len, (struct in6_addr *)(new+ICMP_HEADER_LEN+IP6_SADDR),(struct in6_addr *)(new+ICMP_HEADER_LEN+IP6_DADDR)))
			{
				ready_len += TCP_HEADER_LEN;
				pass_len += TCP_HEADER_LEN;
			}
			break;
		case IPPROTO_UDP:
			if(!udp4to6_trans(new+ready_len,old+pass_len,new_len - ready_len, old_len-pass_len, (struct in6_addr *)(new+ICMP_HEADER_LEN+IP6_SADDR), (struct in6_addr *)(new+ICMP_HEADER_LEN+IP6_DADDR)))
			{
				ready_len += UDP_HEADER_LEN;
				pass_len += UDP_HEADER_LEN;
			}
			break;
		default:break;	
	}
	if (old_len - pass_len + ready_len > new_len) old_len = new_len + pass_len - ready_len;
	memcpy(new+ready_len,old+pass_len,old_len-pass_len);
	newhdr->icmp6_cksum = 0;
	newhdr->icmp6_cksum = csum_ipv6_magic(src6, dst6, new_len, IPPROTO_ICMPV6, csum_partial(new,new_len,0));
	return 0;
}


/*Translate TCP on IPv4 to TCP on IPv6*/

int tcp4to6_trans(unsigned char * new, unsigned char * old, unsigned new_len, unsigned old_len, struct in6_addr * saddr, struct in6_addr * daddr)
{
	
	struct tcphdr * newhdr = (struct tcphdr *) new;
	if(old_len>=TCP_HEADER_LEN)
	{
		if (old_len > new_len) old_len = new_len;
		memcpy(new,old,old_len);
		newhdr->check = 0;
		newhdr->check = csum_ipv6_magic(saddr, daddr, new_len, IPPROTO_TCP, csum_partial(new,new_len,0));
		return 0;
	}
	return 1;
}

/*Translate UDP on IPv4 to UDP on IPv6*/

int udp4to6_trans(unsigned char * new, unsigned char * old, unsigned new_len, unsigned old_len, struct in6_addr * saddr, struct in6_addr * daddr)
{
	struct udphdr * newhdr = (struct udphdr *) new;
	if(old_len>=UDP_HEADER_LEN)
	{
		if (old_len > new_len) old_len = new_len;
		memcpy(new,old,old_len);
		newhdr->check = 0;
		newhdr->check = csum_ipv6_magic(saddr, daddr, new_len, IPPROTO_UDP, csum_partial(new,new_len,0));
		return 0;
	}
	return 1;
}
