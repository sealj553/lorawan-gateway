#ifndef NET_H
#define NET_H

#include "server.h"
#include <stdint.h>

struct sockaddr_in;

void solve_hostname(const char *p_hostname, uint16_t port, struct sockaddr_in *p_sin);
void send_udp(Server server, char *msg, int length);
void fill_pkt_header(char *pkt);
void init_socket(void);

#endif
