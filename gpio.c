#include "spi.h"

#include "libmpsse/mpsse.h"

static struct mpsse_context *dev = NULL;

int gpio_init(){
    return (dev = MPSSE(GPIO, 0, 0)) != NULL && dev->open;
}

void gpio_close(){
    Close(dev);
}

const char* gpio_get_error(){
    return ErrorString(dev);
}
