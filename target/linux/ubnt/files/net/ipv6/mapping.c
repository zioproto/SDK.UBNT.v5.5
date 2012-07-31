/* Stateless Address Mapping IPv6->IPv4
 * IVI Project, CERNET
 */

/*
 *  Packet Mapping from IPv6 to IPv4
 *
 *  "skb" for the old IPv6 sk_buff, grabed from network-layer (ip6_route_input_slow)
 *  "newskb" for new IPv4 packet, with a NULL mac header
 *  entrance of the new packet is "netif_rx()" in /net/core/dev.c
 * 
 *  Ang   2005.9.9
 */

#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <asm/atomic.h>
#include <asm/checksum.h>
#include <linux/in6.h>
#include <linux/in.h>
#include <linux/ipv6.h>
#include <net/ipv6.h>	/* for frag_hdr */
#include <linux/ip.h>
#include <linux/if_ether.h>     /* For ETH_P_IP */
#include <linux/tcp.h>
#include <linux/mapping_config.h>
#include "proto_trans.h"

static void copy_skb_info(struct sk_buff *new, const struct sk_buff *old) {	/* Copy the basic information of skbuff */
	new->sk		= NULL;
	new->dev	= old->dev;
	new->pkt_type	= old->pkt_type;
        atomic_set(&new->users, 1);
	
	/* No data, no header, no protocol, no dst. That's enough. */
}

void data_trans6to4(unsigned char * new, unsigned char * old, unsigned new_len, unsigned old_len, __u32 saddr, __u32 daddr, unsigned short protocol) {
	switch(protocol) {
#ifdef	MAPPING_ICMP 
		case IPPROTO_ICMP:	/* The protocol has already been changed */
			icmp6to4_trans(new, old, new_len, old_len);
			break;
#endif
#ifdef	MAPPING_TCP
		case IPPROTO_TCP:
			tcp6to4_trans(new, old, new_len, old_len, &saddr, &daddr);
			break;
#endif
#ifdef	MAPPING_UDP
		case IPPROTO_UDP:
			udp6to4_trans(new, old, new_len, old_len, &saddr, &daddr);
			break;
#endif
		default:
			if (old_len > new_len) old_len = new_len;
			memcpy(new, old, old_len);
	}
}

int ip6_mapping(struct sk_buff *skb ) {
	struct sk_buff *newskb;
	struct ipv6hdr *oldhdr;
	struct iphdr *newhdr;
	struct frag_hdr *oldhdr_frag = NULL;

	unsigned int newlen;
	unsigned int newpktlen;
	unsigned int oldheaderlen;

	oldhdr = ipv6_hdr(skb);
	if (oldhdr->nexthdr == NEXTHDR_FRAGMENT) {
		oldheaderlen = IP6_HEADER_LEN + 8;
		oldhdr_frag = (struct frag_hdr *)(skb->data + IP6_HEADER_LEN);
	}
	else oldheaderlen = IP6_HEADER_LEN;

	skb->data += oldheaderlen;
	skb->len -= oldheaderlen;

	newpktlen = get_new_pktlen6to4(skb);
	newlen = newpktlen + IP_HEADER_LEN;

	newskb = alloc_skb(newlen + ETHER_HEADER_LEN, GFP_ATOMIC);
	if (newskb == NULL) return 0;

	copy_skb_info(newskb, skb);
	newskb->protocol = htons(ETH_P_IP);
	skb_dst_set(newskb,NULL);
	if (skb_put(newskb, ETHER_HEADER_LEN) == NULL) {
		kfree_skb(newskb);
		return 0;
	}
	skb_reset_mac_header(newskb); /* A NULL Ethernet Header */
	skb_pull(newskb, ETHER_HEADER_LEN);
        if (skb_put(newskb, IP_HEADER_LEN) == NULL) {
                kfree_skb(newskb);
                return 0;
	}
	skb_reset_network_header(newskb);
	newhdr = ip_hdr(newskb);

	iphdr6to4_trans(newhdr, (unsigned char *)oldhdr, newpktlen - skb->len + IP6_HEADER_LEN - oldheaderlen);

	if (skb_put(newskb, newpktlen) == NULL) {
	        kfree_skb(newskb);
	        return 0;
	}
	newskb->dev = dev_get_by_name(dev_net(skb->dev),"eth0"); /* to avoid NULL dereference */
	newskb->pkt_type = PACKET_HOST;
	
	if (oldhdr_frag != NULL && (ntohs(oldhdr_frag->frag_off) & 0x1FFF) != 0)	/* refer to ipv4 mapping.c */
		memcpy(newskb->data + IP_HEADER_LEN, skb->data, skb->len);
	else
		data_trans6to4(newskb->data + IP_HEADER_LEN, skb->data, newpktlen, skb->len, newhdr->saddr, newhdr->daddr, newhdr->protocol);

	skb_dst_set(newskb,NULL);
	//pkt_destroy(oldhdr);

	// for check, added by auron, 2006-4-24
	if (skb_shinfo(newskb)->nr_frags != 0) {
		printk("ip6_mapping error, nr_frags: %d\n", skb_shinfo(newskb)->nr_frags);
		printk("address: %u.%u.%u.%u -> %u.%u.%u.%u\n", NIPQUAD(newhdr->saddr), NIPQUAD(newhdr->daddr));
		printk("protocol: %d\n", ntohs(newhdr->protocol));
		printk("old_len: %d, new_len: %d\n", skb->len, newpktlen);
		skb->data -= oldheaderlen;
		skb->len += oldheaderlen;
		return 0;
	}
	netif_rx(newskb);

	skb->data -= oldheaderlen;
	skb->len += oldheaderlen;
	return 1;
}

