/* Do Protocol Translation IPv6->IPv4
 * IVI Project, CERNET
 * Neil 2005.11.02
 *
 * add:   Path MTU Discovery 2005.11.11  Neil
 * patch: "TCP in ICMP" translation error 2006.4.19 Neil
 */


#ifndef	PROTO_TRANS_H_6TO4
#define PROTO_TRANS_H_6TO4

#include <linux/in6.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/skbuff.h>

extern unsigned short iphdr6to4_trans(struct iphdr *newhdr, unsigned char *oldhdr, int supplement);
extern unsigned get_new_pktlen6to4(struct sk_buff *skb);
extern int icmp6to4_trans(unsigned char *new, unsigned char *old, unsigned new_len, unsigned old_len);
extern int tcp6to4_trans(unsigned char *new, unsigned char *old, unsigned new_len, unsigned old_len, __u32 *saddr, __u32 *daddr);
extern int udp6to4_trans(unsigned char *new, unsigned char *old, unsigned new_len, unsigned old_len, __u32 *saddr, __u32 *daddr);

extern int ip6_mapping(struct sk_buff *skb);	/* Entrance of packet translation */

#endif

