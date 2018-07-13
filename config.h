#ifndef CONFIG_H
#define CONFIG_H

#include "server.h"
#include <stdint.h>

#define bool int
#define true 1
#define false 0

static const char *interface = "eth0";

//Set location in global_conf.json
static const float lat = 42.36339;
static const float lon = -71.09260;
static const int   alt = 60;

//Informal status fields
static const char *platform    = "rpi-single-channel";    //platform definition
static const char *email       = "IoTNet@mit.edu";       //used for contact email
static const char *description = "RPI Single Channel Gateway"; //used for free form description

//Set spreading factor (SF7 - SF12), &nd  center frequency
static const uint16_t bw = 125;
static const uint32_t freq = 916800000;
static const int sf = 7;

bool sx1272 = false;

//Servers
const int numservers = 1;
Server servers[]= { {"router.us.thethings.network", 1700} };

#endif
