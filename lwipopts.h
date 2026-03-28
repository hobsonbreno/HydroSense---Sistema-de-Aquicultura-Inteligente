/**
 * HydroSense v3.0 - Configuração lwIP
 * 
 * Arquivo de opções para a biblioteca lwIP
 */

#ifndef LWIPOPTS_H
#define LWIPOPTS_H

// ============================================================
// Configurações gerais
// ============================================================
#define NO_SYS                          1
#define LWIP_SOCKET                     0
#define LWIP_NETCONN                    0
#define LWIP_PROVIDE_ERRNO              1

// ============================================================
// Configurações de memória
// ============================================================
#define MEM_LIBC_MALLOC                 0
#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        (16 * 1024)
#define MEMP_NUM_TCP_SEG                128
#define MEMP_NUM_ARP_QUEUE              10
#define PBUF_POOL_SIZE                  32
#define LWIP_ARP                        1
#define LWIP_ETHERNET                   1
#define LWIP_ICMP                       1
#define LWIP_RAW                        1
#define TCP_WND                         (16 * TCP_MSS)
#define TCP_MSS                         1460
#define TCP_SND_BUF                     (16 * TCP_MSS)
#define TCP_SND_QUEUELEN                ((8 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
#define LWIP_NETIF_STATUS_CALLBACK      1
#define LWIP_NETIF_LINK_CALLBACK        1
#define LWIP_NETIF_HOSTNAME             1
#define LWIP_NETCONN                    0
#define MEM_STATS                       0
#define SYS_STATS                       0
#define MEMP_STATS                      0
#define LINK_STATS                      0

// ============================================================
// Configurações de DHCP e IP
// ============================================================
#define LWIP_IPV4                       1
#define LWIP_IPV6                       0
#define LWIP_DHCP                       1
#define DHCP_DOES_ARP_CHECK             0
#define LWIP_AUTOIP                     0

// ============================================================
// Configurações TCP/UDP
// ============================================================
#define LWIP_TCP                        1
#define LWIP_UDP                        1
#define LWIP_TCP_KEEPALIVE              1
#define LWIP_NETIF_TX_SINGLE_PBUF       1

// ============================================================
// Configurações HTTP
// ============================================================
#define LWIP_HTTPD                      0
#define LWIP_HTTPD_CGI                  0
#define LWIP_HTTPD_SSI                  0
#define LWIP_HTTPD_SUPPORT_POST         0

// ============================================================
// Configurações DNS
// ============================================================
#define LWIP_DNS                        1
#define DNS_TABLE_SIZE                  4
#define DNS_MAX_NAME_LENGTH             256

// ============================================================
// Configurações de Debug
// ============================================================
#define LWIP_DEBUG                      0
#define LWIP_STATS                      0
#define LWIP_STATS_DISPLAY              0

// ============================================================
// FreeRTOS integration
// ============================================================
#define TCPIP_THREAD_STACKSIZE          2048
#define TCPIP_THREAD_PRIO               (configMAX_PRIORITIES - 1)
#define TCPIP_MBOX_SIZE                 8
#define DEFAULT_RAW_RECVMBOX_SIZE       8
#define DEFAULT_UDP_RECVMBOX_SIZE       8
#define DEFAULT_TCP_RECVMBOX_SIZE       8
#define DEFAULT_ACCEPTMBOX_SIZE         8

// ============================================================
// Otimizações
// ============================================================
#define LWIP_CHECKSUM_CTRL_PER_NETIF    0
#define CHECKSUM_GEN_IP                 1
#define CHECKSUM_GEN_UDP                1
#define CHECKSUM_GEN_TCP                1
#define CHECKSUM_CHECK_IP               1
#define CHECKSUM_CHECK_UDP              1
#define CHECKSUM_CHECK_TCP              1
#define LWIP_CHKSUM_ALGORITHM           3

// ============================================================
// SNTP - Sincronização de Tempo via NTP
// ============================================================
#define SNTP_SERVER_DNS                 1
#define SNTP_MAX_SERVERS                2
#define SNTP_UPDATE_DELAY               3600000  // Atualiza a cada 1 hora

// ============================================================
// Access Point (modo AP)
// ============================================================
#define CYW43_HOST_NAME                 "HydroSense"

#endif /* LWIPOPTS_H */
