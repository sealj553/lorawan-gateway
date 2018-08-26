#include "time_util.h"

#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

uint64_t start_time_us;
uint64_t start_time_ns;

void boot_time(void){
    start_time_us = get_time_us();
    start_time_ns = get_time_ns();
}

uint64_t get_boot_time_ns(void){
    return start_time_ns;
}

uint32_t uptime_us(void){
    return get_time_us() - start_time_us;
}

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

uint64_t get_time_ms(void){
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000000 + now.tv_usec;
}

uint64_t get_time_us(void){
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000000 + now.tv_usec / 1000;
}

uint64_t get_time_ns(void){
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000000000 + now.tv_usec / 1000;
}
