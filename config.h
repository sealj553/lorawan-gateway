#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#define SF_VAL SF_7

#define MODEM_CONFIG1_VAL ((HEADER_MODE_EXPLICIT)|(CODING_RATE_4_5 << 1)|(BW_125KHZ << 4))
#define MODEM_CONFIG2_VAL ((SYMB_TIMEOUT)|(RX_PAYLOAD_CRC_DISABLE << 2)|(TX_CONTINUOUS_DISABLE << 3)|(SF_7 << 4))
#if SF_VAL == 11 || SF_VAL == 12
#define MODEM_CONFIG3_VAL ((ACT_AUTO_OFF << 2)|(LOW_DATA_RATE_OPTIMIZE_ENABLED << 3))
#else
#define MODEM_CONFIG3_VAL ((ACT_AUTO_OFF << 2)|(LOW_DATA_RATE_OPTIMIZE_DISABLED << 3))
#endif

//location
static const float lat              = 42.36339;
static const float lon              = -71.09260;
static const int   alt              = 60;

//informal status fields
static char *platform               = "single-channel-gateway"; //platform definition
static char *email                  = "IoTNet@mit.edu";         //contact email
static char *description            = "Single Channel Gateway"; //free form description

//radio config
static const int bw                 = BW_125KHZ;       //bandwidth (kHz)
//static const uint32_t freq          = 916800000; //center frequency (Hz) for US915 (I think)
//static const uint32_t freq          = 916800000; //center frequency (Hz) for US915 (I think)
static const uint32_t freq          = 903700000;

//servers
//mqtt
static const char *server_hostname  = "router.us.thethings.network";
static const int  server_port       = 1883; //8883 for TLS
static const char *router_id        = "ttn-router-us-west";
//static const char *account_server   = "https://account.thethingsnetwork.org";
//static const char *discovery_server = "discovery.thethings.network:1900";

//udp
//static const char *udp_server       = "router.us.thethings.network:1700";

//gateway config
static const char *gateway_id       = "usb-single-channel-1"; //should be unique
static const char *gateway_key      = "ttn-account-v2.8Gb1eTqVLvtWCrop5JHWYsPmZ1obJCKpAiW76fc7B_lTzX9MNhfxQjLl3Mb38VRPiNji1DJN6XQRKuf90kMkjQ"; //secret!

//The gateway's frequency plan: one of EU_863_870, US_902_928, CN_779_787, EU_433, AU_915_928, CN_470_510, AS_923, AS_920_923, AS_923_925, KR_920_923
static char *frequency_plan         = "US_902_928";
static char *data_rate              = "SF9BW125"; //generate this in future
//static char *coding_rate            = "4/5";
static const int coding_rate = CODING_RATE_4_5;

//update interval (seconds)
static const unsigned int update_interval = 30;

#endif

/*Host: <Region>.thethings.network, where <Region> is last part of the handler you registered your application to, e.g. eu.
Port: 1883 or 8883 for TLS
PEM encoded CA certificate for TLS: mqtt-ca.pem
Note: When this certificate expires, we will migrate to Let’s Encrypt certificates. Therefore you might want to include the Let’s Encrypt Roots in your certificate chain.
Username: Application ID
Password: Application Access Key */
