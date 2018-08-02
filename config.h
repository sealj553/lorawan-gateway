#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

//location
static const float lat              = 42.36339;
static const float lon              = -71.09260;
static const int   alt              = 60;

//informal status fields
static char *platform         = "single-channel-gateway";     //platform definition
static char *email            = "IoTNet@mit.edu";             //contact email
static char *description      = "RPI Single Channel Gateway"; //free form description

//radio config
static const uint16_t bw            = 125;       //bandwidth (kHz)
static const uint32_t freq          = 916800000; //center frequency (Hz) for US915, I think...
static const int sf                 = 7;         //spreading factor (SF7-SF12)
static const int rf_chain           = 5; //?

//servers
//mqtt
static const char *account_server   = "https://account.thethingsnetwork.org";
static const char *discovery_server = "discovery.thethings.network:1900";
static const char *router_id        = "ttn-router-eu"; //change to us?
static const int  router_port       = 1883;
//udp
static const char *udp_server       = "router.us.thethings.network:1700";

//gateway config
static const char *gateway_id       = "usb-single-channel-1";
static const char *gateway_key      = "ttn-account-v2.URLdw1lxJJNbyVulb1Q8A4L3vfLQhmLr0ifIC2ElEFTBBkMOgBtvxRslhRGXWV5fUD25TxP551G8M9RYS_BgpQ"; //I better hide this...

//The gateway's frequency plan: one of EU_863_870, US_902_928, CN_779_787, EU_433, AU_915_928, CN_470_510, AS_923, AS_920_923, AS_923_925, KR_920_923
static char *frequency_plan = "US_902_928";
static char *data_rate = "SF9BW125";
static char *coding_rate = "4/5";

//protocol to use
typedef enum { SEMTECH_UDP, PROTOBUF_MQTT } Protocol;
static const Protocol protocol = PROTOBUF_MQTT;

//update interval in seconds
static const unsigned int update_interval = 30;

#endif
