#include "net.h"

#include "registers.h"
#include "server.h"

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char *interface = "eth0";
struct sockaddr_in si_other;
int sock;
struct ifreq ifr;

void solve_hostname(const char *p_hostname, uint16_t port, struct sockaddr_in *p_sin){
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    char service[6] = { '\0' };
    snprintf(service, 6, "%hu", port);

    struct addrinfo *p_result = NULL;

    //resolve the domain name into a list of addresses
    int error = getaddrinfo(p_hostname, service, &hints, &p_result);
    if(error != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        exit(EXIT_FAILURE);
    }

    //loop over all returned results
    for(struct addrinfo *p_rp = p_result; p_rp != NULL; p_rp = p_rp->ai_next) {
        struct sockaddr_in *p_saddr = (struct sockaddr_in*)p_rp->ai_addr;
        //printf("%s solved to %s\n", p_hostname, inet_ntoa(p_saddr->sin_addr));
        p_sin->sin_addr = p_saddr->sin_addr;
    }

    freeaddrinfo(p_result);
}

void send_udp(Server server, char *msg, int length){
    si_other.sin_port = htons(server.port);
    solve_hostname(server.address, server.port, &si_other);
    if(sendto(sock, msg, length, 0, (struct sockaddr*)&si_other, sizeof(si_other)) == -1){
        perror("sendto()");
        exit(1);
    }
}

void prepare_socket(void){
    //prepare Socket connection
    if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        perror("socket");
        exit(1);
    }

    memset(&si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    ioctl(sock, SIOCGIFHWADDR, &ifr);

    //ID based on MAC Adddress of eth0
    printf("Gateway ID: %.2x:%.2x:%.2x:ff:ff:%.2x:%.2x:%.2x\n",
            ifr.ifr_hwaddr.sa_data[0],
            ifr.ifr_hwaddr.sa_data[1],
            ifr.ifr_hwaddr.sa_data[2],
            ifr.ifr_hwaddr.sa_data[3],
            ifr.ifr_hwaddr.sa_data[4],
            ifr.ifr_hwaddr.sa_data[5]);
}

void fill_pkt_header(char *pkt){
    pkt[0]  = PROTOCOL_VERSION;
    pkt[1]  = rand(); //random tokens
    pkt[2]  = rand();
    pkt[3]  = PKT_PUSH_DATA;
    pkt[4]  = ifr.ifr_hwaddr.sa_data[0];
    pkt[5]  = ifr.ifr_hwaddr.sa_data[1];
    pkt[6]  = ifr.ifr_hwaddr.sa_data[2];
    pkt[7]  = 0xFF;
    pkt[8]  = 0xFF;
    pkt[9]  = ifr.ifr_hwaddr.sa_data[3];
    pkt[10] = ifr.ifr_hwaddr.sa_data[4];
    pkt[11] = ifr.ifr_hwaddr.sa_data[5];
}
