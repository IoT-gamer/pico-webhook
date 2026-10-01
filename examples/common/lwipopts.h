#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Core LwIP Configuration for Pico W (Background/Polling Mode)
#define NO_SYS                      1
#define LWIP_SOCKET                 0
#define LWIP_COMPAT_SOCKETS         0
#define LWIP_NETCONN                0
#define LWIP_IPV4                   1
#define LWIP_IPV6                   0
#define LWIP_DHCP                   1
#define LWIP_DNS                    1
#define LWIP_TCP                    1
#define LWIP_UDP                    1
#define LWIP_IGMP                   1

// Memory and Hardware Settings
#define MEM_LIBC_MALLOC             0
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    4000
#define MEMP_NUM_TCP_SEG            32
#define PBUF_POOL_SIZE              24
#define LWIP_TX_PBUF_BOUNCE         1
#define LWIP_NETIF_TX_SINGLE_PBUF   1

// TCP Buffer and Window Settings
#define TCP_MSS                     1460
#define TCP_WND                     32768 // Required for large TLS records
#define TCP_SND_BUF                 (8 * TCP_MSS)
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))

// Callbacks and Network Interfaces
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_CHKSUM_ALGORITHM       3

// Application Layer TCP (ALTCP) and TLS
#define LWIP_ALTCP                  1
#define LWIP_ALTCP_TLS              1
#define LWIP_ALTCP_TLS_MBEDTLS      1

#endif