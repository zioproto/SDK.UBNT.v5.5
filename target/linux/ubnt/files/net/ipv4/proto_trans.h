/* Do Protocol Translation IPv4->IPv6
 * IVI Project, CERNET
 * Neil 2005.11.02
 *
 * add:   Path MTU Discovery 2005.11.11  Neil
 * patch: "TCP in ICMP" translation error 2006.4.19 Neil
 */

#ifndef	PROTO_TRANS_H_4TO6
#define PROTO_TRANS_H_4TO6
#include <linux/in.h>
#include <linux/in6.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/skbuff.h>

extern unsigned short iphdr4to6_trans(unsigned char *newhdr, struct iphdr *oldhdr, __u32 src_prefix, __u32 dst_prefix, int supplement, int fragment);
extern unsigned get_new_pktlen4to6(struct sk_buff *skb);
extern unsigned get_new_headerlen4to6(struct iphdr *oldhdr);
extern int icmp4to6_trans(unsigned char *new, unsigned char *old, unsigned new_len, unsigned old_len, __u32 src_prefix, __u32 dst_prefix, struct in6_addr *src6, struct in6_addr *dst6);
extern int tcp4to6_trans(unsigned char *new, unsigned char *old, unsigned new_len, unsigned old_len, struct in6_addr *saddr, struct in6_addr *daddr);
extern int udp4to6_trans(unsigned char *new, unsigned char *old, unsigned new_len, unsigned old_len, struct in6_addr *saddr, struct in6_addr *daddr);

extern int ip_mapping(struct sk_buff *skb, __u32 src_prefix, __u32 dst_prefix);	/* Entrance of packet translation */
		

#endif
