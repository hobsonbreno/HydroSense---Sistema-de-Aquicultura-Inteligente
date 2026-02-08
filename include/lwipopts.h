#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Configuração do lwIP para Pico W

// Otimizações
#define NO_SYS                          1
#define LWIP_SOCKET                     0
#define LWIP_NETCONN                    0

// Memória
#define MEM_LIBC_MALLOC                 0
#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        16000
#define MEMP_NUM_TCP_SEG                32
#define MEMP_NUM_ARP_QUEUE              10
#define PBUF_POOL_SIZE                  24
#define LWIP_ARP                        1
#define LWIP_ETHERNET                   1
#define LWIP_ICMP                       1
#define LWIP_RAW                        0
#define TCP_WND                         (8 * TCP_MSS)
#define TCP_MSS                         1460
#define TCP_SND_BUF                     (8 * TCP_MSS)
#define TCP_SND_QUEUELEN                ((4 * (TCP_SND_BUF) + (TCP_MSS - 1))/(TCP_MSS))
#define LWIP_NETIF_STATUS_CALLBACK      1
#define LWIP_NETIF_LINK_CALLBACK        1
#define LWIP_NETIF_HOSTNAME             1
#define LWIP_NETIF_TX_SINGLE_PBUF       1
#define DHCP_DOES_ARP_CHECK             0
#define LWIP_DHCP                       1
#define LWIP_UDP                        1
#define LWIP_TCP                        1
#define LWIP_TCP_KEEPALIVE              1
#define LWIP_DNS                        1
#define LWIP_IGMP                       0

// Threads (não usamos)
#define LWIP_TCPIP_CORE_LOCKING         0
#define SYS_LIGHTWEIGHT_PROT            0

// Estatísticas
#define LWIP_STATS                      0
#define LWIP_STATS_DISPLAY              0

// Checksum
#define CHECKSUM_GEN_IP                 1
#define CHECKSUM_GEN_UDP                1
#define CHECKSUM_GEN_TCP                1
#define CHECKSUM_GEN_ICMP               1
#define CHECKSUM_CHECK_IP               1
#define CHECKSUM_CHECK_UDP              1
#define CHECKSUM_CHECK_TCP              1
#define CHECKSUM_CHECK_ICMP             1
#define LWIP_CHKSUM_ALGORITHM           3

// Debug (desabilitado)
#define LWIP_DEBUG                      0

#endif
