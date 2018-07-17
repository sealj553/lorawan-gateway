#include "time_util.h"

#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

int seconds(void){
    struct timeval nowtime;
    gettimeofday(&nowtime, NULL);
    return nowtime.tv_sec;
}

void delay(unsigned int ms){
    struct timespec time;
    time.tv_sec = ms / 1000;
    if(time.tv_sec > 0){
        ms -= time.tv_sec * 1000;
    }
    time.tv_nsec = ms * 1000000L;
    nanosleep(&time, NULL);
}

