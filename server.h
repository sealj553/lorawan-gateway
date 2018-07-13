#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>

typedef struct server {
    const char *address;
    uint16_t port;
} Server;

#endif
