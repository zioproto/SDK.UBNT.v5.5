/*	This file includes some basic constants used by the mapping
 *	codes, and the configuration of mapping facility: what should
 *	be mapped, while what should not.
 *	Maybe one day the configuration could be added to the kernel
 *	configuration, and that's cool and clean.
 *
 *	Ang
 *	2005-11-3
 */
#ifndef MAPPING_CONFIG_H
#define MAPPING_CONFIG_H

#define NO_PROTO                 0
#define ETHER_HEADER_LEN	14
#define IP_HEADER_LEN		20
#define IP6_HEADER_LEN		40
#define IPV6_HEADER_LEN		IP6_HEADER_LEN	/* for compatibility */
#define ICMP_HEADER_LEN          8
#define TCP_HEADER_LEN          20 
#define UDP_HEADER_LEN           8
#define IP_ICMP_LEN             28
#define IP6_ICMP_LEN            48
#define IP_SADDR                12
#define IP_DADDR                16
#define IP6_SADDR                8
#define IP6_DADDR               24

/*******************************************************************
 * Configuration
 * *****************************************************************/
#undef MAPPING_FRAGMENTATION	 
#define	MAPPING_ICMP		 1
#define	MAPPING_TCP		 1	
#define	MAPPING_UDP		 1
#undef  MAPPING_ADDRESS_CHECKSUM 
#define MAPPING_EXT_HEADER	 1

#endif

