#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE

#include "registers.h"
#include "config.h"
 
#include "base64.h"
#include "spi.h"
#include "gpio.h"
#include "time_util.h"

#include <jansson.h>

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netdb.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

struct sockaddr_in si_other;
int sock;
struct ifreq ifr;

uint32_t cp_nb_rx_rcv;
uint32_t cp_nb_rx_ok;
uint32_t cp_nb_rx_ok_tot;
uint32_t cp_nb_rx_bad;
uint32_t cp_nb_rx_nocrc;
uint32_t cp_up_pkt_fwd;

void PrintConfiguration();
void Die(const char *s);
bool ReceivePkt(char* payload, uint8_t* p_length);
void SetupLoRa();
void SolveHostname(const char* p_hostname, uint16_t port, struct sockaddr_in* p_sin);
void SendUdp(char *msg, int length);
void SendStat();
bool Receivepacket();

void Die(const char *s){
    perror(s);
    exit(1);
}

bool ReceivePkt(char *payload, uint8_t *p_length){
    //clear rxDone
    WriteRegister(REG_IRQ_FLAGS, 0x40);

    int irqflags = ReadRegister(REG_IRQ_FLAGS);
    cp_nb_rx_rcv++;

    //payload crc: 0x20
    if((irqflags & 0x20) == 0x20) {
        printf("CRC error\n");
        WriteRegister(REG_IRQ_FLAGS, 0x20);
        return false;
    } else {
        ++cp_nb_rx_ok;
        ++cp_nb_rx_ok_tot;

        uint8_t currentAddr = ReadRegister(REG_FIFO_RX_CURRENT_ADDR);
        uint8_t receivedCount = ReadRegister(REG_RX_NB_BYTES);
        *p_length = receivedCount;

        WriteRegister(REG_FIFO_ADDR_PTR, currentAddr);

        for(int i = 0; i < receivedCount; ++i){
            payload[i] = ReadRegister(REG_FIFO);
        }
    }
    return true;
}

void SetupLoRa(){

    //digitalWrite(RST, HIGH);
    //delay(100);
    //digitalWrite(RST, LOW);
    //delay(100);

    uint8_t version = ReadRegister(REG_VERSION);
    if(version != 0x12){ 
        printf("Transceiver version 0x%02X\n", version);
        Die("Unrecognized transceiver");
    } else {
        printf("SX1276 detected\n");
    }

    WriteRegister(REG_OPMODE, SX72_MODE_SLEEP);

    // set frequency
    uint64_t frf = ((uint64_t)freq << 19) / 32000000;
    WriteRegister(REG_FRF_MSB, frf >> 16);
    WriteRegister(REG_FRF_MID, frf >> 8);
    WriteRegister(REG_FRF_LSB, frf >> 0);

    WriteRegister(REG_SYNC_WORD, 0x34); //LoRaWAN public sync word

    if(sf == 11 || sf == 12){
        WriteRegister(REG_MODEM_CONFIG3, 0x0C);
    } else {
        WriteRegister(REG_MODEM_CONFIG3, 0x04);
    }
    WriteRegister(REG_MODEM_CONFIG, 0x72);
    WriteRegister(REG_MODEM_CONFIG2, (sf << 4) | 0x04);

    if(sf == 10 || sf == 11 || sf == 12){
        WriteRegister(REG_SYMB_TIMEOUT_LSB, 0x05);
    } else {
        WriteRegister(REG_SYMB_TIMEOUT_LSB, 0x08);
    }
    WriteRegister(REG_MAX_PAYLOAD_LENGTH, 0x80);
    WriteRegister(REG_PAYLOAD_LENGTH, PAYLOAD_LENGTH);
    WriteRegister(REG_HOP_PERIOD, 0xFF);
    WriteRegister(REG_FIFO_ADDR_PTR, ReadRegister(REG_FIFO_RX_BASE_AD));

    //set Continous Receive Mode
    WriteRegister(REG_LNA, LNA_MAX_GAIN); //max lna gain
    WriteRegister(REG_OPMODE, SX72_MODE_RX_CONTINUOS);
}

void SolveHostname(const char *p_hostname, uint16_t port, struct sockaddr_in *p_sin){
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    char service[6] = { '\0' };
    snprintf(service, 6, "%hu", port);

    struct addrinfo* p_result = NULL;

    //Resolve the domain name into a list of addresses
    int error = getaddrinfo(p_hostname, service, &hints, &p_result);
    if(error != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        exit(EXIT_FAILURE);
    }

    //Loop over all returned results
    for(struct addrinfo *p_rp = p_result; p_rp != NULL; p_rp = p_rp->ai_next) {
        struct sockaddr_in *p_saddr = (struct sockaddr_in*)p_rp->ai_addr;
        //printf("%s solved to %s\n", p_hostname, inet_ntoa(p_saddr->sin_addr));
        p_sin->sin_addr = p_saddr->sin_addr;
    }

    freeaddrinfo(p_result);
}

void SendUdp(char *msg, int length){
    for(int i = 0; i < numservers; ++i){
        si_other.sin_port = htons(servers[i].port);

        SolveHostname(servers[i].address, servers[i].port, &si_other);
        if(sendto(sock, (char*)msg, length, 0, (struct sockaddr*) &si_other, sizeof(si_other)) == -1){
            Die("sendto()");
        }
    }
}

void SendStat(){
    static char status_report[STATUS_SIZE]; /* status report as a JSON object */
    char stat_timestamp[24];

    int stat_index = 0;

    /* pre-fill the data buffer with fixed fields */
    status_report[0]  = PROTOCOL_VERSION;
    status_report[3]  = PKT_PUSH_DATA;
    status_report[4]  = ifr.ifr_hwaddr.sa_data[0];
    status_report[5]  = ifr.ifr_hwaddr.sa_data[1];
    status_report[6]  = ifr.ifr_hwaddr.sa_data[2];
    status_report[7]  = 0xFF;
    status_report[8]  = 0xFF;
    status_report[9]  = ifr.ifr_hwaddr.sa_data[3];
    status_report[10] = ifr.ifr_hwaddr.sa_data[4];
    status_report[11] = ifr.ifr_hwaddr.sa_data[5];

    //start composing datagram with the header
    uint8_t token_h = rand(); //random token
    uint8_t token_l = rand();
    status_report[1] = token_h;
    status_report[2] = token_l;
    stat_index = 12; //12 byte header

    //get timestamp for statistics
    time_t t = time(NULL);
    strftime(stat_timestamp, sizeof stat_timestamp, "%F %T %Z", gmtime(&t));

    //compare to original implementation to see if this actually works
    //TODO: finish this format string
    //TODO: add object at beginning
    json_t *root = json_pack("{s:s,s:i ",
            "time", stat_timestamp, //string
            "lati", lat,            //double
            "long", lon,            //double
            "alti", alt,            //double
            "rxnb", cp_nb_rx_rcv,   //uint
            "rxok", cp_nb_rx_ok,    //uint
            "rxfw", cp_up_pkt_fwd,  //uint
            "ackr", 0,              //double
            "dwnb", 0,              //uint
            "txnb", 0,              //uint
            "pfrm", platform,       //string
            "mail", email,          //string
            "desc", description);   //string

    if(!root){
        printf("Unable to create json object!\n");    
    }

    const char *json = json_string_value(root);
    printf("stat update: %s\n", json);

    printf("stat update: %s", stat_timestamp);
    if (cp_nb_rx_ok_tot==0) {
        printf(" no packet received yet\n");
    } else {
        printf(" %u packet%sreceived\n", cp_nb_rx_ok_tot, cp_nb_rx_ok_tot>1?"s ":" ");
    }

    //build and send message.
    memcpy(status_report + 12, json, json_string_length(root));
    SendUdp(status_report, stat_index + json_string_length(root));
}

bool Receivepacket(){
    bool packet_received = false;

    //if (digitalRead(dio0) == 1) {
    //TODO:fix this
    if(1){
        long int SNR;
        char message[256];
        uint8_t length = 0;
        if(ReceivePkt(message, &length)){
            packet_received = true;

            uint8_t value = ReadRegister(REG_PKT_SNR_VALUE);
            if(value & 0x80){ //the SNR sign bit is 1
                //invert and divide by 4
                value = ((~value + 1) & 0xFF) >> 2;
                SNR = -value;
            } else {
                // Divide by 4
                SNR = (value & 0xFF) >> 2;
            }

            int rssicorr = 157;

            printf("Packet RSSI: %d, ", ReadRegister(0x1A) - rssicorr);
            printf("RSSI: %d, ", ReadRegister(0x1B) - rssicorr);
            printf("SNR: %li, ", SNR);
            printf("Length: %hhu Message:'", length);
            for(int i = 0; i < length; ++i){
                printf("%c", isprint(message[i]) ? message[i] : '.');
            }
            printf("'\n");

            char buff_up[TX_BUFF_SIZE]; //buffer to compose the upstream packet
            int buff_index = 0;

            /* gateway <-> MAC protocol variables */
            //static uint32_t net_mac_h; /* Most Significant Nibble, network order */
            //static uint32_t net_mac_l; /* Least Significant Nibble, network order */

            /* pre-fill the data buffer with fixed fields */
            buff_up[0] = PROTOCOL_VERSION;
            buff_up[3] = PKT_PUSH_DATA;

            /* process some of the configuration variables */
            //net_mac_h = htonl((uint32_t)(0xFFFFFFFF & (lgwm>>32)));
            //net_mac_l = htonl((uint32_t)(0xFFFFFFFF &  lgwm  ));
            //*(uint32_t *)(buff_up + 4) = net_mac_h; 
            //*(uint32_t *)(buff_up + 8) = net_mac_l;

            buff_up[4]  = ifr.ifr_hwaddr.sa_data[0];
            buff_up[5]  = ifr.ifr_hwaddr.sa_data[1];
            buff_up[6]  = ifr.ifr_hwaddr.sa_data[2]; 
            buff_up[7]  = 0xFF;
            buff_up[8]  = 0xFF;
            buff_up[9]  = ifr.ifr_hwaddr.sa_data[3];
            buff_up[10] = ifr.ifr_hwaddr.sa_data[4];
            buff_up[11] = ifr.ifr_hwaddr.sa_data[5];

            //start composing datagram with the header
            uint8_t token_h = rand(); //random token
            uint8_t token_l = rand();
            buff_up[1] = token_h;
            buff_up[2] = token_l;
            buff_index = 12; //12 byte header

            //TODO: tmst can jump is time is (re)set, not good
            struct timeval now;
            gettimeofday(&now, NULL);
            uint32_t tmst = now.tv_sec * 1000000 + now.tv_usec;

            //encode payload
            char b64[BASE64_MAX_LENGTH];
            bin_to_b64((uint8_t*)message, length, b64, BASE64_MAX_LENGTH);

            //TODO:fix this
            json_t *js_obj = json_object();

            json_object_set(js_obj, "", json_pack(
                        "tmst", tmst,                 //uint
                        "freq", (double)freq/1000000, //double
                        "chan", 0,                    //uint
                        "rfch", 0,                    //uint
                        "stat", 1,                    //uint
                        "modu", "LORA", "datr",       //??
                        "codr", "4/5",                //string
                        "rssi", ReadRegister(0x1A) - rssicorr, //int
                        "lsnr", SNR,                  //double
                        "size", length,               //uint
                        "data", b64));                //string

            json_t *js_arr = json_array();
            json_array_append(js_arr, js_obj);

            json_t *js_obj2 = json_object();
            json_object_set(js_obj2, "rxpk", js_arr);

            //TODO: add json stuff here
            //TODO: finish format specifier and params
            json_t *root = json_pack("", "rxpk", js_arr);


            ////string json = sb.GetString();
            ////printf("rxpk update: %s\n", json.c_str());

            ////// Build and send message.
            ////memcpy(buff_up + 12, json.c_str(), json.size());
            ////SendUdp(buff_up, buff_index + json.size());

            fflush(stdout);
        }
    }
    return packet_received;
}

void PrintConfiguration(){
    for(int i = 0; i < numservers; ++i){
        printf("server: address = %s; port = %hu\n", servers[i].address, servers[i].port);
    }
    printf("Gateway Configuration:\n");
    printf("  platform=%s, email=%s, desc=%s\n", platform, email, description);
    printf("  lat=%.8f, lon=%.8f, alt=%d\n", lat, lon, alt);
    printf("  freq=%d, sf=%d\n", freq, sf);
}

int main(){
    PrintConfiguration();

    int rstPin = gpioInit("/sys/class/gpio/gpio3/value", O_WRONLY);
    int intPin = gpioInit("/sys/class/gpio/gpio4/value", O_RDONLY);

    //TODO:fix this
    /*if(!spi_init()){
      fprintf(stderr, "Failed to initialize SPI interface: %s\n", spi_get_error());
      exit(1);
      }

      if(!gpio_init()){
      fprintf(stderr, "Failed to initialize GPIO interface: %s\n", gpio_get_error());
      exit(1);
      }*/

    // Init SPI

    // Setup LORA
    SetupLoRa();

    // Prepare Socket connection
    if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        Die("socket");
    }

    memset(&si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    ioctl(sock, SIOCGIFHWADDR, &ifr);

    // ID based on MAC Adddress of eth0
    printf("Gateway ID: %.2x:%.2x:%.2x:ff:ff:%.2x:%.2x:%.2x\n",
            ifr.ifr_hwaddr.sa_data[0],
            ifr.ifr_hwaddr.sa_data[1],
            ifr.ifr_hwaddr.sa_data[2],
            ifr.ifr_hwaddr.sa_data[3],
            ifr.ifr_hwaddr.sa_data[4],
            ifr.ifr_hwaddr.sa_data[5]
          );

    printf("Listening at SF%i on %.6lf Mhz.\n", sf,(double)freq/1000000);
    printf("-----------------------------------\n");

    uint32_t lasttime;
    while(1){
        //Packet received ?
        if(Receivepacket()){
            printf("Packet received!\n");
        }

        struct timeval nowtime;
        gettimeofday(&nowtime, NULL);
        uint32_t nowseconds = nowtime.tv_sec;
        if(nowseconds - lasttime >= 30){
            lasttime = nowseconds;
            SendStat();
            cp_nb_rx_rcv  = 0;
            cp_nb_rx_ok   = 0;
            cp_up_pkt_fwd = 0;
        }

        // Let some time to the OS
        delay(1);
    }
}
