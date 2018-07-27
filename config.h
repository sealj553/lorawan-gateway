#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

//location
static const float lat              = 42.36339;
static const float lon              = -71.09260;
static const int   alt              = 60;

//informal status fields
static const char *platform         = "single-channel-gateway";     //platform definition
static const char *email            = "IoTNet@mit.edu";             //used for contact email
static const char *description      = "RPI Single Channel Gateway"; //used for free form description

//radio config
static const uint16_t bw            = 125;
static const uint32_t freq          = 916800000; //center frequency for US915, I think...
static const int sf                 = 7; //spreading factor (SF7-SF12)

//servers
static const char *account_server   = "https://account.thethingsnetwork.org";
static const char *discovery_server = "discovery.thethings.network:1900";
static const char *router_id        = "ttn-router-eu";

//gateway config
static const char *gateway_id       = "usb-single-channel";
static const char *gateway_key      = "ttn-account-v2.URLdw1lxJJNbyVulb1Q8A4L3vfLQhmLr0ifIC2ElEFTBBkMOgBtvxRslhRGXWV5fUD25TxP551G8M9RYS_BgpQ";

//update interval in seconds
static const unsigned int update_interval = 30;

#endif
