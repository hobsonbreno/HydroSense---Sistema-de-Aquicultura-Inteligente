/**
 * HydroSense - Implementação Servidor DHCP
 * 
 * Servidor DHCP simples para atribuir IPs aos clientes do AP
 */

#include "dhcpserver.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include <string.h>
#include <stdio.h>

static dhcp_server_t* dhcp_server_ptr = NULL;

static uint8_t* dhcp_get_option(dhcp_msg_t* msg, uint8_t option) {
    uint8_t* opt = msg->options + 4; // Skip magic cookie
    while (*opt != DHCP_OPT_END && opt < msg->options + sizeof(msg->options)) {
        if (*opt == option) {
            return opt + 2; // Skip option code and length
        }
        opt += 2 + opt[1]; // Move to next option
    }
    return NULL;
}

static void dhcp_recv_callback(void* arg, struct udp_pcb* pcb, struct pbuf* p, 
                                const ip_addr_t* addr, u16_t port) {
    dhcp_server_t* d = (dhcp_server_t*)arg;
    if (p == NULL || p->len < sizeof(dhcp_msg_t) - 312) {
        if (p) pbuf_free(p);
        return;
    }
    
    dhcp_msg_t* request = (dhcp_msg_t*)p->payload;
    
    // Verifica se é um request válido
    if (request->op != DHCP_BOOTREQUEST) {
        pbuf_free(p);
        return;
    }
    
    // Obtém tipo da mensagem
    uint8_t* msg_type_ptr = dhcp_get_option(request, DHCP_OPT_MSG_TYPE);
    if (msg_type_ptr == NULL) {
        pbuf_free(p);
        return;
    }
    uint8_t msg_type = *msg_type_ptr;
    
    // Prepara resposta
    dhcp_msg_t response;
    memset(&response, 0, sizeof(response));
    
    response.op = DHCP_BOOTREPLY;
    response.htype = request->htype;
    response.hlen = request->hlen;
    response.xid = request->xid;
    memcpy(response.chaddr, request->chaddr, 16);
    
    // Atribui IP ao cliente (192.168.4.X onde X = 100 + next_ip)
    uint8_t client_ip = 100 + d->next_ip;
    d->next_ip = (d->next_ip + 1) % DHCP_MAX_CLIENTS;
    
    response.yiaddr[0] = 192;
    response.yiaddr[1] = 168;
    response.yiaddr[2] = 4;
    response.yiaddr[3] = client_ip;
    
    // Server IP
    response.siaddr[0] = 192;
    response.siaddr[1] = 168;
    response.siaddr[2] = 4;
    response.siaddr[3] = 1;
    
    // Magic cookie
    response.options[0] = 99;
    response.options[1] = 130;
    response.options[2] = 83;
    response.options[3] = 99;
    
    uint8_t* opt = response.options + 4;
    
    // Message type
    *opt++ = DHCP_OPT_MSG_TYPE;
    *opt++ = 1;
    *opt++ = (msg_type == DHCP_DISCOVER) ? DHCP_OFFER : DHCP_ACK;
    
    // Server ID
    *opt++ = DHCP_OPT_SERVER_ID;
    *opt++ = 4;
    *opt++ = 192; *opt++ = 168; *opt++ = 4; *opt++ = 1;
    
    // Lease time (1 hora)
    *opt++ = DHCP_OPT_LEASE_TIME;
    *opt++ = 4;
    *opt++ = 0; *opt++ = 0; *opt++ = 0x0E; *opt++ = 0x10; // 3600 seconds
    
    // Subnet mask
    *opt++ = DHCP_OPT_SUBNET_MASK;
    *opt++ = 4;
    *opt++ = 255; *opt++ = 255; *opt++ = 255; *opt++ = 0;
    
    // Router (gateway)
    *opt++ = DHCP_OPT_ROUTER;
    *opt++ = 4;
    *opt++ = 192; *opt++ = 168; *opt++ = 4; *opt++ = 1;
    
    // DNS (mesmo que gateway)
    *opt++ = DHCP_OPT_DNS;
    *opt++ = 4;
    *opt++ = 192; *opt++ = 168; *opt++ = 4; *opt++ = 1;
    
    // End
    *opt++ = DHCP_OPT_END;
    
    pbuf_free(p);
    
    // Envia resposta
    struct pbuf* resp_p = pbuf_alloc(PBUF_TRANSPORT, sizeof(dhcp_msg_t), PBUF_RAM);
    if (resp_p != NULL) {
        memcpy(resp_p->payload, &response, sizeof(dhcp_msg_t));
        
        ip_addr_t broadcast;
        IP_ADDR4(&broadcast, 255, 255, 255, 255);
        
        udp_sendto(pcb, resp_p, &broadcast, DHCP_CLIENT_PORT);
        pbuf_free(resp_p);
        
        printf("📡 DHCP: Cliente conectado -> 192.168.4.%d\n", client_ip);
    }
}

void dhcp_server_init(dhcp_server_t* d, ip4_addr_t* ip, ip4_addr_t* mask) {
    printf("🔧 Iniciando servidor DHCP...\n");
    
    d->ip = *ip;
    d->mask = *mask;
    d->next_ip = 0;
    
    d->udp = udp_new();
    if (d->udp == NULL) {
        printf("❌ Falha ao criar UDP para DHCP\n");
        return;
    }
    
    err_t err = udp_bind(d->udp, IP_ADDR_ANY, DHCP_SERVER_PORT);
    if (err != ERR_OK) {
        printf("❌ Falha no bind DHCP (err=%d)\n", err);
        udp_remove(d->udp);
        d->udp = NULL;
        return;
    }
    
    udp_recv(d->udp, dhcp_recv_callback, d);
    dhcp_server_ptr = d;
    
    printf("✅ Servidor DHCP iniciado (pool: 192.168.4.100-103)\n");
}

void dhcp_server_deinit(dhcp_server_t* d) {
    if (d->udp != NULL) {
        udp_remove(d->udp);
        d->udp = NULL;
    }
    dhcp_server_ptr = NULL;
}
