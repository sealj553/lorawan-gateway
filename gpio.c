#include "spi.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int gpioInit(const char *dev, mode_t mode){
    int fd;
    if((fd = open(dev, mode)) == -1){
        perror("open");
        return -1;
    }
    return fd;
}

void gpioClose(int fd){
    if(close(fd) == -1){
        perror("close");
    }
}

int gpioRead(int fd){
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

int gpioWrite(int fd, int value){
    if(write(fd, value ? "1" : "0", 1) == -1){
        perror("write");
        return -1;
    }
    return value;
}
