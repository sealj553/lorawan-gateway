#ifndef CONFIG_H
#define CONFIG_H

#include "server.h"
#include <stdint.h>
#include <stdbool.h>

//location
static const float lat = 42.36339;
static const float lon = -71.09260;
static const int   alt = 60;

//informal status fields
static const char *platform    = "single-channel-gateway";     //platform definition
static const char *email       = "IoTNet@mit.edu";             //used for contact email
static const char *description = "RPI Single Channel Gateway"; //used for free form description

//spreading factor (SF7 - SF12), center frequency
static const uint16_t bw = 125;
static const uint32_t freq = 916800000;
static const int sf = 7;

//servers
static const int numservers = 2;
static Server servers[]= { {"router.us.thethings.network", 1700},
                           {"iot.caribae.us", 8000} };

//update interval in seconds
static const unsigned int update_interval = 10;

#endif
