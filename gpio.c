#include "spi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

int irq;
struct pollfd fds;

int gpio_init(const char *devname, mode_t mode){
    int fd;
    if((fd = open(devname, mode)) == -1){
        perror("open");
        exit(1);
    }
    return fd;
}

void gpio_close(int fd){
    if(close(fd) == -1){
        perror("close");
    }
}

int gpio_read(int fd){
    char buf;

    if(read(fd, &buf, 1) == -1){
        perror("read");
        return -1;
    }

    if(lseek(fd, 0, SEEK_SET) == -1){
        perror("lseek");
        return -1;
    }

    return buf == '1';
}

int gpio_write(int fd, int value){
    if(write(fd, value ? "1" : "0", 1) == -1){
        perror("write");
        return -1;
    }
    return value;
}

void setup_interrupt(const char *edge){
    int fd;
    if((fd = open("/sys/class/gpio/gpio4/edge", O_WRONLY)) == -1){
        perror("open");
        exit(1);
    }
    if(write(fd, edge, strlen(edge)) == -1){
        perror("write");
        exit(1);
    }
    close(fd); 

    irq = gpio_init("/sys/class/gpio/gpio4/value", O_RDONLY);

    fds.fd = irq;
    fds.events = POLLPRI;
}

void wait_irq(void){
    //wait until rising edge on gpio4
    if(poll(&fds, 1, -1) == -1){
        perror("poll");
        exit(1);
    }
}
