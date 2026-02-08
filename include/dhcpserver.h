/**
 * HydroSense - Servidor DHCP simples para modo AP
 * 
 * Atribui IPs aos clientes que conectam no Access Point
 */

#ifndef DHCPSERVER_H
#define DHCPSERVER_H

#include "lwip/ip4_addr.h"
#include "lwip/udp.h"

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define DHCP_MAX_CLIENTS 4

typedef struct {
    struct udp_pcb* udp;
    ip4_addr_t ip;
    ip4_addr_t mask;
    uint8_t next_ip;
} dhcp_server_t;

// Estrutura do pacote DHCP
typedef struct {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint8_t ciaddr[4];
    uint8_t yiaddr[4];
    uint8_t siaddr[4];
    uint8_t giaddr[4];
    uint8_t chaddr[16];
    uint8_t sname[64];
    uint8_t file[128];
    uint8_t options[312];
} __attribute__((packed)) dhcp_msg_t;

// Opcodes DHCP
#define DHCP_BOOTREQUEST 1
#define DHCP_BOOTREPLY   2

// Message types
#define DHCP_DISCOVER 1
#define DHCP_OFFER    2
#define DHCP_REQUEST  3
#define DHCP_DECLINE  4
#define DHCP_ACK      5
#define DHCP_NAK      6
#define DHCP_RELEASE  7
#define DHCP_INFORM   8

// Options
#define DHCP_OPT_MSG_TYPE     53
#define DHCP_OPT_SERVER_ID    54
#define DHCP_OPT_LEASE_TIME   51
#define DHCP_OPT_SUBNET_MASK  1
#define DHCP_OPT_ROUTER       3
#define DHCP_OPT_DNS          6
#define DHCP_OPT_END          255

void dhcp_server_init(dhcp_server_t* d, ip4_addr_t* ip, ip4_addr_t* mask);
void dhcp_server_deinit(dhcp_server_t* d);

#endif // DHCPSERVER_H
