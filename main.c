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

uint32_t cp_nb_rx_rcv    = 0;
uint32_t cp_nb_rx_ok     = 0;
uint32_t cp_nb_rx_ok_tot = 0;
uint32_t cp_nb_rx_bad    = 0;
uint32_t cp_nb_rx_nocrc  = 0;
uint32_t cp_up_pkt_fwd   = 0;

int rstPin, intPin, spi;

void print_configuration();
void die(const char *s);
bool receive_pkt(char *payload, uint8_t *p_length);
void setup_lora();
void solve_hostname(const char *p_hostname, uint16_t port, struct sockaddr_in *p_sin);
void send_udp(char *msg, int length);
void send_stat();
bool receive_packet();

void die(const char *s){
    perror(s);
    exit(1);
}

bool receive_pkt(char *payload, uint8_t *p_length){
    //clear rxDone
    spi_write_reg(spi, REG_IRQ_FLAGS, PAYLOAD_LENGTH);

    int irqflags = spi_read_reg(spi, REG_IRQ_FLAGS);
    ++cp_nb_rx_rcv;

    if((irqflags & PAYLOAD_CRC) == PAYLOAD_CRC) {
        puts("CRC error");
        spi_write_reg(spi, REG_IRQ_FLAGS, PAYLOAD_CRC);
        return false;
    } else {
        ++cp_nb_rx_ok;
        ++cp_nb_rx_ok_tot;

        uint8_t currentAddr = spi_read_reg(spi, REG_FIFO_RX_CURRENT_ADDR);
        uint8_t receivedCount = spi_read_reg(spi, REG_RX_NB_BYTES);
        *p_length = receivedCount;

        spi_write_reg(spi, REG_FIFO_ADDR_PTR, currentAddr);

        for(int i = 0; i < receivedCount; ++i){
            payload[i] = spi_read_reg(spi, REG_FIFO);
        }
    }
    return true;
}

void setup_lora(){
    gpio_write(rstPin, 0);
    delay(100);
    gpio_write(rstPin, 1);
    delay(100);

    uint8_t version = spi_read_reg(spi, REG_VERSION);

    printf("Transceiver version 0x%02X, ", version);
    if(version != 0x12){ 
        puts("Unrecognized transceiver");
        exit(1);
    } else {
        puts("SX1276 detected\n");
    }

    spi_write_reg(spi, REG_OPMODE, SX72_MODE_SLEEP);

    // set frequency
    uint64_t frf = ((uint64_t)freq << 19) / 32000000;
    spi_write_reg(spi, REG_FRF_MSB, frf >> 16);
    spi_write_reg(spi, REG_FRF_MID, frf >> 8);
    spi_write_reg(spi, REG_FRF_LSB, frf >> 0);

    spi_write_reg(spi, REG_SYNC_WORD, 0x34); //LoRaWAN public sync word

    if(sf == 11 || sf == 12){
        spi_write_reg(spi, REG_MODEM_CONFIG3, 0x0C);
    } else {
        spi_write_reg(spi, REG_MODEM_CONFIG3, 0x04);
    }
    spi_write_reg(spi, REG_MODEM_CONFIG, 0x72);
    spi_write_reg(spi, REG_MODEM_CONFIG2, (sf << 4) | 0x04);

    if(sf == 10 || sf == 11 || sf == 12){
        spi_write_reg(spi, REG_SYMB_TIMEOUT_LSB, 0x05);
    } else {
        spi_write_reg(spi, REG_SYMB_TIMEOUT_LSB, 0x08);
    }
    spi_write_reg(spi, REG_MAX_PAYLOAD_LENGTH, 0x80);
    spi_write_reg(spi, REG_PAYLOAD_LENGTH, PAYLOAD_LENGTH);
    spi_write_reg(spi, REG_HOP_PERIOD, 0xFF);
    spi_write_reg(spi, REG_FIFO_ADDR_PTR, spi_read_reg(spi, REG_FIFO_RX_BASE_AD));

    //set Continous Receive Mode
    spi_write_reg(spi, REG_LNA, LNA_MAX_GAIN); //max lna gain
    spi_write_reg(spi, REG_OPMODE, SX72_MODE_RX_CONTINUOS);
}

void solve_hostname(const char *p_hostname, uint16_t port, struct sockaddr_in *p_sin){
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    char service[6] = { '\0' };
    snprintf(service, 6, "%hu", port);

    struct addrinfo *p_result = NULL;

    //resolve the domain name into a list of addresses
    int error = getaddrinfo(p_hostname, service, &hints, &p_result);
    if(error != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        exit(EXIT_FAILURE);
    }

    //loop over all returned results
    for(struct addrinfo *p_rp = p_result; p_rp != NULL; p_rp = p_rp->ai_next) {
        struct sockaddr_in *p_saddr = (struct sockaddr_in*)p_rp->ai_addr;
        //printf("%s solved to %s\n", p_hostname, inet_ntoa(p_saddr->sin_addr));
        p_sin->sin_addr = p_saddr->sin_addr;
    }

    freeaddrinfo(p_result);
}

void send_udp(char *msg, int length){
    for(int i = 0; i < numservers; ++i){
        si_other.sin_port = htons(servers[i].port);

        solve_hostname(servers[i].address, servers[i].port, &si_other);
        if(sendto(sock, (char*)msg, length, 0, (struct sockaddr*) &si_other, sizeof(si_other)) == -1){
            die("sendto()");
        }
    }
}

void send_stat(){
    static char status_report[STATUS_SIZE]; //status report as a JSON object

    //pre-fill the data buffer with fixed fields
    status_report[0]  = PROTOCOL_VERSION;
    status_report[1]  = rand(); //random tokens
    status_report[2]  = rand();
    status_report[3]  = PKT_PUSH_DATA;
    status_report[4]  = ifr.ifr_hwaddr.sa_data[0];
    status_report[5]  = ifr.ifr_hwaddr.sa_data[1];
    status_report[6]  = ifr.ifr_hwaddr.sa_data[2];
    status_report[7]  = 0xFF;
    status_report[8]  = 0xFF;
    status_report[9]  = ifr.ifr_hwaddr.sa_data[3];
    status_report[10] = ifr.ifr_hwaddr.sa_data[4];
    status_report[11] = ifr.ifr_hwaddr.sa_data[5];

    //get timestamp for statistics
    char stat_timestamp[24];
    time_t t = time(NULL);
    strftime(stat_timestamp, sizeof stat_timestamp, "%F %T %Z", gmtime(&t));

    json_t *obj = json_pack("{ss,sf,sf,si,si,si,si,sf,si,si,ss,ss,ss}",
            "time", stat_timestamp, //string
            "lati", lat,            //double
            "long", lon,            //double
            "alti", alt,            //int
            "rxnb", cp_nb_rx_rcv,   //uint
            "rxok", cp_nb_rx_ok,    //uint
            "rxfw", cp_up_pkt_fwd,  //uint
            "ackr", 0.f,            //double
            "dwnb", 0,              //uint
            "txnb", 0,              //uint
            "pfrm", platform,       //string
            "mail", email,          //string
            "desc", description);   //string

    json_t *root = json_object();
    json_object_set_new(root, "stat", obj);

    if(!root){
        puts("Unable to create json object!");    
    }

    const char *json_str = json_dumps(root, JSON_COMPACT);
    printf("stat update: %s\n", json_str);

    printf("stat update: %s", stat_timestamp);
    if(cp_nb_rx_ok_tot == 0){
        printf(" no packet received yet\n");
    } else {
        printf(" %u packet%sreceived\n", cp_nb_rx_ok_tot, cp_nb_rx_ok_tot>1?"s ":" ");
    }

    //build and send message
    //12 is header size
    memcpy(status_report + 12, json_str, json_string_length(root));
    send_udp(status_report, 12 + json_string_length(root));

    json_decref(root);
}

bool receive_packet(){
    bool packet_received = false;

    if(gpio_read(intPin)){
        long int SNR;
        char message[256];
        uint8_t length = 0;
        if(receive_pkt(message, &length)){
            packet_received = true;

            uint8_t value = spi_read_reg(spi, REG_PKT_SNR_VALUE);
            if(value & 0x80){ //the SNR sign bit is 1
                //invert and divide by 4
                value = ((~value + 1) & 0xFF) >> 2;
                SNR = -value;
            } else {
                // Divide by 4
                SNR = (value & 0xFF) >> 2;
            }

            int rssicorr = 157;

            printf("Packet RSSI: %d, ", spi_read_reg(spi, 0x1A) - rssicorr);
            printf("RSSI: %d, ", spi_read_reg(spi, 0x1B) - rssicorr);
            printf("SNR: %li, ", SNR);
            printf("Length: %hhu Message:'", length);
            for(int i = 0; i < length; ++i){
                printf("%c", isprint(message[i]) ? message[i] : '.');
            }
            printf("'\n");

            //char buff_up[TX_BUFF_SIZE]; //buffer to compose the upstream packet
            ////int buff_index = 0;

            /* gateway <-> MAC protocol variables */
            //static uint32_t net_mac_h; /* Most Significant Nibble, network order */
            //static uint32_t net_mac_l; /* Least Significant Nibble, network order */

            /* pre-fill the data buffer with fixed fields */
            ////buff_up[0] = PROTOCOL_VERSION;
            ////buff_up[3] = PKT_PUSH_DATA;

            /* process some of the configuration variables */
            //net_mac_h = htonl((uint32_t)(0xFFFFFFFF & (lgwm>>32)));
            //net_mac_l = htonl((uint32_t)(0xFFFFFFFF &  lgwm  ));
            //*(uint32_t *)(buff_up + 4) = net_mac_h; 
            //*(uint32_t *)(buff_up + 8) = net_mac_l;

            /////buff_up[4]  = ifr.ifr_hwaddr.sa_data[0];
            /////buff_up[5]  = ifr.ifr_hwaddr.sa_data[1];
            /////buff_up[6]  = ifr.ifr_hwaddr.sa_data[2]; 
            /////buff_up[7]  = 0xFF;
            /////buff_up[8]  = 0xFF;
            /////buff_up[9]  = ifr.ifr_hwaddr.sa_data[3];
            /////buff_up[10] = ifr.ifr_hwaddr.sa_data[4];
            /////buff_up[11] = ifr.ifr_hwaddr.sa_data[5];

            ///////start composing datagram with the header
            /////uint8_t token_h = rand(); //random token
            /////uint8_t token_l = rand();
            /////buff_up[1] = token_h;
            /////buff_up[2] = token_l;
            /////buff_index = 12; //12 byte header

///            //TODO: start_time can jump is time is (re)set, not good
///            struct timeval now;
///            gettimeofday(&now, NULL);
///            uint32_t start_time = now.tv_sec * 1000000 + now.tv_usec;
///
///            //encode payload
///            char b64[BASE64_MAX_LENGTH];
///            bin_to_b64((uint8_t*)message, length, b64, BASE64_MAX_LENGTH);
///
            //TODO:fix this
            //json_t *val = json_object();

            //////json_object_set(val, "s", json_pack("idk"));

            ////stat
            //json_object_set(val, "", json_pack(
            //            "tmst", start_time,           //uint
            //            "freq", (double)freq/1000000, //double
            //            "chan", 0,                    //uint
            //            "rfch", 0,                    //uint
            //            "stat", 1,                    //uint
            //            "modu", "LORA", "datr",       //??
            //            "codr", "4/5",                //string
            //            "rssi", spi_read_reg(spi, 0x1A) - rssicorr, //int
            //            "lsnr", SNR,                  //double
            //            "size", length,               //uint
            //            "data", b64));                //string

            //json_t *js_arr = json_array();
            //json_array_append(js_arr, val);

            //json_t *js_obj2 = json_object();
            //json_object_set(js_obj2, "rxpk", js_arr);

            //TODO: add json stuff here
            //TODO: finish format specifier and params
            /////json_t *root = json_pack("", "rxpk", js_arr);

            //print_json(root);

            ////string json = sb.GetString();
            ////printf("rxpk update: %s\n", json.c_str());

            ////// Build and send message.
            ////memcpy(buff_up + 12, json.c_str(), json.size());
            ////send_udp(buff_up, buff_index + json.size());

            ///json_decref(root);
            fflush(stdout);
        }
    }
    return packet_received;
}

void print_configuration(){
    for(int i = 0; i < numservers; ++i){
        printf("server: address = %s; port = %hu\n", servers[i].address, servers[i].port);
    }
    printf("Gateway Configuration:\n");
    printf("  platform=%s, email=%s, desc=%s\n", platform, email, description);
    printf("  lat=%.8f, lon=%.8f, alt=%d\n", lat, lon, alt);
    printf("  freq=%d, sf=%d\n", freq, sf);
}

int main(){

    //set up hardware
    setup_interrupt("rising"); //gpio4, input
    rstPin = gpio_init("/sys/class/gpio/gpio3/value", O_WRONLY); //gpio 3, output
    spi = spi_init("/dev/spidev0.0", O_RDWR);

    //setup LoRa
    setup_lora();

    print_configuration();

    //prepare Socket connection
    if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        die("socket");
    }

    memset(&si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    ioctl(sock, SIOCGIFHWADDR, &ifr);

    //ID based on MAC Adddress of eth0
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
    send_stat();

    uint32_t lasttime;
    while(1){
        //Packet received ?
        if(receive_packet()){
            printf("Packet received!\n");
        }

        struct timeval nowtime;
        gettimeofday(&nowtime, NULL);
        uint32_t nowseconds = nowtime.tv_sec;
        if(nowseconds - lasttime >= 30){
            lasttime = nowseconds;
            send_stat();
            cp_nb_rx_rcv  = 0;
            cp_nb_rx_ok   = 0;
            cp_up_pkt_fwd = 0;
        }

        //Let some time to the OS
        //delay(1);
    }
}
