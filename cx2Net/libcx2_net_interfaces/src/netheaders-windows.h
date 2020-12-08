#ifndef NETHEADERSWINDOWS_H
#define NETHEADERSWINDOWS_H

#ifdef WIN32

#include <stdint.h>



// Derived from linux/if_ether.h (GPLv2.0)
#define ETH_ALEN        6
#define ETH_P_IP	0x0800		/* Internet Protocol packet	*/

struct  ethhdr {
        unsigned char   h_dest[ETH_ALEN];       /* destination eth addr */
        unsigned char   h_source[ETH_ALEN];     /* source ether addr    */
        uint16_t        h_proto;                /* packet type ID field */
} __attribute__((packed));

// Derived from linux/ip.h  (GPLv2.0)

struct iphdr {
#if __ORDER_LITTLE_ENDIAN__
    uint8_t	ihl:4,
        version:4;
#else
    uint8_t	version:4,
        ihl:4;
#endif
    uint8_t	tos;
    uint16_t	tot_len;
    uint16_t	id;
    uint16_t	frag_off;
    uint8_t	ttl;
    uint8_t	protocol;
    uint16_t	check;
    uint32_t	saddr;
    uint32_t	daddr;
    /*The options start here. */
};

// Derived from linux/tcp.h  (GPLv2.0)
struct tcphdr {
    uint16_t	source;
    uint16_t	dest;
    uint32_t	seq;
    uint32_t	ack_seq;
#if __ORDER_LITTLE_ENDIAN__
    uint16_t	res1:4,
        doff:4,
        fin:1,
        syn:1,
        rst:1,
        psh:1,
        ack:1,
        urg:1,
        ece:1,
        cwr:1;
#else
    uint16_t	doff:4,
        res1:4,
        cwr:1,
        ece:1,
        urg:1,
        ack:1,
        psh:1,
        rst:1,
        syn:1,
        fin:1;
#endif
    uint16_t	window;
    uint16_t	check;
    uint16_t	urg_ptr;
};



#endif


#endif // NETHEADERSWINDOWS_H
