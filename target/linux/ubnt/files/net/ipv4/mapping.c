/* Stateless Address Mapping IPv4->IPv6
 * IVI Project, CERNET
 */

/*
 *  Packet Mapping from IPv4 to IPv6
 *
 *  "skb" for the old IPv4 sk_buff, grabed from network-layer (ip_route_input)
 *  "newskb" for new IPv6 packet, with a NULL mac header
 *  entrance of the new packet is "netif_rx()" in /net/core/dev.c
 * 
 *  Ang   2005.9.9
 */

#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <asm/atomic.h>
#include <linux/in6.h>
#include <linux/in.h>
#include <linux/ipv6.h>
#include <net/ipv6.h>
#include <linux/if_ether.h>	/* For ETH_P_IPV6 */
#include <linux/tcp.h>
#include <asm/checksum.h>
#include <linux/mapping_config.h>
#include "proto_trans.h"

static void copy_skb_info(struct sk_buff *new, const struct sk_buff *old) {	/* Copy the basic information of skbuff */
	new->sk		= NULL;
	new->dev	= old->dev;
	new->pkt_type	= old->pkt_type;
        atomic_set(&new->users, 1);
	
	/* No data, no header, no protocol, no dst. That's enough. */
}

void data_trans4to6(unsigned char * new, unsigned char * old, unsigned new_len, unsigned old_len, struct in6_addr *saddr, struct in6_addr *daddr, __u32 src_prefix, __u32 dst_prefix, unsigned short nexthdr) {	/* Translator of the payload */
	switch (nexthdr) {
#ifdef	MAPPING_ICMP
		case IPPROTO_ICMPV6:	/* The nexthdr has already been translated */
			icmp4to6_trans(new, old, new_len, old_len, src_prefix, dst_prefix, saddr, daddr);
			break;
#endif
#ifdef	MAPPING_TCP
		case IPPROTO_TCP:
			tcp4to6_trans(new, old, new_len, old_len, saddr, daddr);
			break;
#endif
#ifdef	MAPPING_UDP
		case IPPROTO_UDP:
			udp4to6_trans(new, old, new_len, old_len, saddr, daddr);
			break;
#endif
		default:
			if (old_len > new_len) old_len = new_len;
			memcpy(new, old, old_len);
	}
}

int ip_mapping(struct sk_buff *skb, __u32 src_prefix, __u32 dst_prefix) { /* Note: empty mac header, some problems may come from here */
        unsigned int newlen;
        unsigned int newpktlen;
	unsigned int newheaderlen;
	unsigned int nexthdr;
        struct sk_buff *newskb;
        struct ipv6hdr *newhdr;
        struct iphdr *oldhdr;
	
	oldhdr = ip_hdr(skb);
	skb->data += IP_HEADER_LEN;
	skb->len -= IP_HEADER_LEN;

	newheaderlen = get_new_headerlen4to6(oldhdr);
	newpktlen = get_new_pktlen4to6(skb);
	newlen = newpktlen + newheaderlen;

        newskb = alloc_skb(newlen + ETHER_HEADER_LEN, GFP_ATOMIC);

	if (newskb == NULL) return 0;
	copy_skb_info(newskb, skb);
	newskb->protocol = htons(ETH_P_IPV6);	/* New ipv6 packet */
	skb_dst_set(newskb,NULL);
	skb_put(newskb, ETHER_HEADER_LEN);	/* To add a fake ethernet header */
	skb_reset_mac_header(newskb);
	skb_pull(newskb, ETHER_HEADER_LEN);
	skb_put(newskb, newheaderlen);
	skb_reset_network_header(newskb);
	newhdr = ipv6_hdr(newskb);
	nexthdr = iphdr4to6_trans((unsigned char *)newhdr, oldhdr, src_prefix, dst_prefix, newpktlen - skb->len + newheaderlen - IPV6_HEADER_LEN, newheaderlen - IPV6_HEADER_LEN);

	if (skb_put(newskb, newpktlen) == NULL) {
		kfree_skb(newskb);
		return 0;
	}
	
	if ((ntohs(oldhdr->frag_off) & 0x1FFF) != 0)	/* for fragment, we don't need a translation of trans-layer header & icmp header */
		memcpy(newskb->data + newheaderlen, skb->data, skb->len);
	else       	
		data_trans4to6(newskb->data + newheaderlen, skb->data, newpktlen, skb->len, &(newhdr->saddr), &(newhdr->daddr), src_prefix, dst_prefix, nexthdr);

	if (!newskb->dev) newskb->dev = dev_net(skb->dev)->loopback_dev;	/* for local out */

	netif_rx_ni(newskb);
	skb->data -= IP_HEADER_LEN;
	skb->len += IP_HEADER_LEN;
	return 1;
}

