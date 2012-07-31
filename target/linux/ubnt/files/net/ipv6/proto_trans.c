/* Do Protocol Translation IPv6->IPv4
 * IVI Project, CERNET
 * Neil 2005.11.02
 *
 * add:   Path MTU Discovery 2005.11.11  Neil
 * patch: "TCP in ICMP" translation error 2006.4.19 Neil
 */

#include <linux/in6.h>
#include <linux/in.h>
#include <linux/ipv6.h>
#include <net/ipv6.h>	/* For frag_hdr */
#include <linux/ip.h>
#include <linux/icmp.h>
#include <linux/icmpv6.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <asm/atomic.h>
#include <asm/checksum.h>
#include <linux/mapping_config.h>
#include "proto_trans.h"
	
void mapping_address6to4(__u32 *newv4, const struct in6_addr oldv6) {
	unsigned char *startpos;
#ifndef		MAPPING_ADDRESS_CHECKSUM
#if defined(__LITTLE_ENDIAN_BITFIELD)
	startpos = (unsigned char *)(oldv6.s6_addr32 + 1);
	if (*startpos != 0xFF) *newv4 = htonl((10 << 24) + 4538);
	else *newv4 = *((__u32 *)(startpos + 1));
#elif defined(__BIG_ENDIAN_BITFIELD)
	*newv4 = oldv6.s6_addr32[0];
#else
#error  "Please fix <asm/byteorder.h>"
#endif
#else
	if (ip_compute_csum((unsigned char *)oldv6.s6_addr32, 16) == 0)
		*newv4 = oldv6.s6_addr32[3];
	else
		*newv4 = htonl((10 << 24) + 4538);
#endif
}

__u32 getfragmentation(const struct in6_addr oldv6) {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	return oldv6.s6_addr32[2];
#elif defined(__BIG_ENDIAN_BITFIELD)
	return oldv6.s6_addr32[1];
#else
#error 	"Please fix <asm/byteorder.h>"
#endif
}

unsigned short iphdr6to4_trans(struct iphdr *newhdr, unsigned char *olddata, int supplement) {
	struct ipv6hdr *oldhdr = (struct ipv6hdr *)olddata;
	newhdr->version  = 4;
	newhdr->ihl      = 5;
	newhdr->tos      = 0;   /* Normal TOS       */
	newhdr->frag_off = 0;
	newhdr->id	 = 0;
	newhdr->protocol = oldhdr->nexthdr;

#ifdef  MAPPING_EXT_HEADER
	if (oldhdr->nexthdr == NEXTHDR_FRAGMENT) {
		struct frag_hdr *ext_header = (struct frag_hdr *)(olddata + IPV6_HEADER_LEN);
		newhdr->protocol = ext_header->nexthdr;
		newhdr->frag_off = ext_header->frag_off;
		newhdr->id = ext_header->identification;
	}
	else
		newhdr->frag_off = htons(0x4000);
#endif
		
#ifdef	MAPPING_ICMP
	if (newhdr->protocol == IPPROTO_ICMPV6)
		newhdr->protocol = IPPROTO_ICMP;
#endif
	newhdr->ttl	 = oldhdr->hop_limit;

#ifdef	MAPPING_FRAGMENTATION
	*((__u32 *)(&(newhdr->id))) = getfragmentation(oldhdr->saddr);
#endif

	newhdr->check	 = 0;
	
	newhdr->tot_len  = htons(ntohs(oldhdr->payload_len) + IP_HEADER_LEN + supplement);

	mapping_address6to4(&(newhdr->saddr), oldhdr->saddr);
	mapping_address6to4(&(newhdr->daddr), oldhdr->daddr);
	newhdr->check    = ip_fast_csum((unsigned char *)newhdr, newhdr->ihl);
	//printk("mapping6->4:%u.%u.%u.%u -> %u.%u.%u.%u\n", NIPQUAD(newhdr->saddr), NIPQUAD(newhdr->daddr));	
	return newhdr->protocol;
}

/*Get new IP datagram length*/

unsigned get_new_pktlen6to4(struct sk_buff * skb)
{
	struct ipv6hdr * hdr = ipv6_hdr(skb);
	struct icmp6hdr * ihdr = (struct icmp6hdr *)skb->data;
	int nexthdr;
	short int offset = 0;
	if ( hdr->nexthdr == NEXTHDR_FRAGMENT) {
		struct frag_hdr *ext_hdr = (struct frag_hdr *)((unsigned char *)(ipv6_hdr(skb)) + IPV6_HEADER_LEN);
		nexthdr = ext_hdr->nexthdr;
		offset = ntohs(ext_hdr->frag_off) & 0x1FFF;
	}
	else nexthdr = hdr->nexthdr;

	if ( nexthdr != IPPROTO_ICMPV6 )
		return skb->len;
	else if ( offset != 0 )
		return skb->len;
	else if ( ihdr->icmp6_type == ICMPV6_ECHO_REQUEST || ihdr->icmp6_type == ICMPV6_ECHO_REPLY )
		return skb->len;
	else {
		struct ipv6hdr * hdr_in_pkt = (struct ipv6hdr *)(skb->data + ICMP_HEADER_LEN);
		if (hdr_in_pkt->nexthdr == NEXTHDR_FRAGMENT)
			return skb->len - 28;
		else
			return skb->len - 20;
	}
}


/*Translate ICMPv6 to ICMP*/

int icmp6to4_trans(unsigned char * new, unsigned char * old, unsigned new_len, unsigned old_len)
{
	struct icmp6hdr * oldhdr = (struct icmp6hdr *) old;
	struct icmphdr * newhdr = (struct icmphdr *) new;
	struct ipv6hdr * hdr_in_pkt;
	unsigned short newproto = NO_PROTO;
	unsigned ready_len = 0;
	unsigned pass_len = 0;
	
	if ( old_len < ICMP_HEADER_LEN )
		return 1;
	
	switch(oldhdr->icmp6_type)
	{
		case ICMPV6_ECHO_REPLY:
			newhdr->type=ICMP_ECHOREPLY;
			ready_len += ICMP_HEADER_LEN;
			pass_len += ICMP_HEADER_LEN;
			break;
		case ICMPV6_ECHO_REQUEST:
			newhdr->type=ICMP_ECHO;
			ready_len += ICMP_HEADER_LEN;
			pass_len += ICMP_HEADER_LEN;
			break;
		case ICMPV6_DEST_UNREACH:
			newhdr->type=ICMP_DEST_UNREACH;
			if (oldhdr->icmp6_code == 4) 
				oldhdr->icmp6_code = 3; /* for port unreach. */
			if (old_len >= IP6_ICMP_LEN)
			{
				ready_len += IP_ICMP_LEN;
				hdr_in_pkt = (struct ipv6hdr *)(old + ICMP_HEADER_LEN);
				if (hdr_in_pkt->nexthdr == NEXTHDR_FRAGMENT) 
				{
					newproto=iphdr6to4_trans((struct iphdr *)(new + ICMP_HEADER_LEN),(old + ICMP_HEADER_LEN), -8);
					pass_len += (IP6_ICMP_LEN + 8);
				}
				else 
				{
					newproto=iphdr6to4_trans((struct iphdr *)(new + ICMP_HEADER_LEN),(old + ICMP_HEADER_LEN), 0);
					pass_len += IP6_ICMP_LEN;
				}
			}
			else {
				pass_len += ICMP_HEADER_LEN;
				ready_len += ICMP_HEADER_LEN;
			}
			break;
		case ICMPV6_TIME_EXCEED:
			newhdr->type=ICMP_TIME_EXCEEDED;
			if (old_len >= IP6_ICMP_LEN)
			{
				hdr_in_pkt = (struct ipv6hdr *)(old + ICMP_HEADER_LEN);
				ready_len += IP_ICMP_LEN;
				if (hdr_in_pkt->nexthdr == NEXTHDR_FRAGMENT) 
				{
					newproto=iphdr6to4_trans((struct iphdr *)(new + ICMP_HEADER_LEN),(old + ICMP_HEADER_LEN), -8);
					pass_len += (IP6_ICMP_LEN + 8);
				}
				else
				{
					newproto=iphdr6to4_trans((struct iphdr *)(new + ICMP_HEADER_LEN),(old + ICMP_HEADER_LEN), 0);
					pass_len += IP6_ICMP_LEN;
				}
			}
			else
			{
				pass_len += ICMP_HEADER_LEN;
				ready_len += ICMP_HEADER_LEN;
			}
			break;
		case ICMPV6_PKT_TOOBIG:
			newhdr->type=ICMP_DEST_UNREACH;
			oldhdr->icmp6_code = ICMP_FRAG_NEEDED;
			if (old_len >= IP6_ICMP_LEN)
			{
				hdr_in_pkt = (struct ipv6hdr *)(old + ICMP_HEADER_LEN);
				ready_len += IP_ICMP_LEN;
				if (hdr_in_pkt->nexthdr == NEXTHDR_FRAGMENT) {
					newproto=iphdr6to4_trans((struct iphdr *)(new + ICMP_HEADER_LEN),(old + ICMP_HEADER_LEN), -8);
					oldhdr->icmp6_dataun.un_data32[0]=htonl(ntohl(oldhdr->icmp6_dataun.un_data32[0])-28);
					pass_len += (IP6_ICMP_LEN + 8);
				}
				else
				{
					oldhdr->icmp6_dataun.un_data32[0]=htonl(ntohl(oldhdr->icmp6_dataun.un_data32[0])-20);
					newproto=iphdr6to4_trans((struct iphdr *)(new + ICMP_HEADER_LEN),(old + ICMP_HEADER_LEN), 0);
					pass_len += IP6_ICMP_LEN;
				}
			}
			else
			{
				pass_len += ICMP_HEADER_LEN;
				ready_len += ICMP_HEADER_LEN;
			}
		default:break;
	}
	newhdr->code = oldhdr->icmp6_code;
	newhdr->un.gateway = oldhdr->icmp6_dataun.un_data32[0];
	switch(newproto)
	{
		case NO_PROTO:
			break;
		case IPPROTO_ICMP:
			if(!icmp6to4_trans(new+ready_len,old+pass_len,new_len - ready_len, old_len-pass_len))
			{
				ready_len += ICMP_HEADER_LEN;
				pass_len += ICMP_HEADER_LEN;
			}
			break;
		case IPPROTO_TCP:
			if(!tcp6to4_trans(new+ready_len,old+pass_len,new_len - ready_len, old_len-pass_len, (__u32 *)(new+ICMP_HEADER_LEN+IP_SADDR),(__u32 *)(new+ICMP_HEADER_LEN+IP_DADDR)))
			{
				ready_len += TCP_HEADER_LEN;
				pass_len += TCP_HEADER_LEN;
			}
			break;
		case IPPROTO_UDP:
			if(!udp6to4_trans(new+ready_len,old+pass_len,new_len - ready_len, old_len-pass_len, (__u32 *)(new+ICMP_HEADER_LEN+IP_SADDR),(__u32 *)(new+ICMP_HEADER_LEN+IP_DADDR)))
			{
				ready_len += UDP_HEADER_LEN;
				pass_len += UDP_HEADER_LEN;
			}
			break;
		default:break;	
	}
	/* memcpy should not run out of skb->end */
	if (ready_len + old_len - pass_len > new_len) 
		old_len = new_len + pass_len - ready_len;
	memcpy(new+ready_len,old+pass_len,old_len-pass_len);
	newhdr->checksum = 0;
	newhdr->checksum = ip_compute_csum(new,new_len);
	return 0;
}


/*Translate TCP on IPv6 to TCP on IPv4*/

int tcp6to4_trans(unsigned char * new, unsigned char * old, unsigned new_len, unsigned old_len, __u32 * saddr, __u32 * daddr)
{
	struct tcphdr * newhdr = (struct tcphdr *) new;
	if ( old_len >= TCP_HEADER_LEN )
	{
		if (old_len > new_len) old_len = new_len;
		memcpy(new,old,old_len);
		newhdr->check = 0;
		newhdr->check = csum_tcpudp_magic(*saddr, *daddr, new_len, IPPROTO_TCP, csum_partial(new,new_len,0));
		return 0;
	}
	return 1;
}

/*Translate UDP on IPv6 to UDP on IPv4*/

int udp6to4_trans(unsigned char * new, unsigned char * old, unsigned new_len, unsigned old_len, __u32 * saddr, __u32 * daddr)
{
	struct udphdr * newhdr = (struct udphdr *) new;
	if ( old_len >= UDP_HEADER_LEN )
	{
		if (old_len > new_len) old_len = new_len;
		memcpy(new,old,old_len);
		newhdr->check = 0;
		newhdr->check = csum_tcpudp_magic(*saddr, *daddr, new_len, IPPROTO_UDP, csum_partial(new,new_len,0));
		return 0;
	}
	return 1;
}
