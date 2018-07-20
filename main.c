#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE

#include "registers.h"
#include "config.h"
#include "base64.h"
#include "spi.h"
#include "gpio.h"
#include "time_util.h"
#include "net.h"

#include <jansson.h>

#include <sys/time.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

uint32_t cp_nb_rx_rcv  = 0;
uint32_t cp_nb_rx_ok   = 0;
uint32_t cp_up_pkt_fwd = 0;

int rstPin, intPin;
const double mhz = (double)freq/1000000;

void print_configuration();
void die(const char *s);
bool read_data(char *payload, uint8_t *p_length);
void setup_lora();
void send_stat();
bool receive_packet(void);
void init(void);
void send_ack(const char *message);

void die(const char *s){
    perror(s);
    exit(1);
}

bool read_data(char *payload, uint8_t *p_length){
    //clear rxDone
    spi_write_reg(REG_IRQ_FLAGS, PAYLOAD_LENGTH);

    int irqflags = spi_read_reg(REG_IRQ_FLAGS);
    ++cp_nb_rx_rcv;

    if((irqflags & PAYLOAD_CRC) == PAYLOAD_CRC) {
        puts("CRC error");
        spi_write_reg(REG_IRQ_FLAGS, PAYLOAD_CRC);
        return false;
    }

    ++cp_nb_rx_ok;

    uint8_t receivedCount = spi_read_reg(REG_RX_NB_BYTES);
    *p_length = receivedCount;

    spi_write_reg(REG_FIFO_ADDR_PTR, spi_read_reg(REG_FIFO_RX_CURRENT_ADDR));

    for(int i = 0; i < receivedCount; ++i){
        payload[i] = spi_read_reg(REG_FIFO);
    }

    return true;
}

size_t write_data(const char *buffer, int size){
    //idle/standby mode
    spi_write_reg(REG_OPMODE, MODE_LONG_RANGE_MODE | MODE_STDBY);

    // reset FIFO address and paload length
    spi_write_reg(REG_FIFO_ADDR_PTR, 0);
    spi_write_reg(REG_PAYLOAD_LENGTH, 0);

    int currentLength = spi_read_reg(REG_PAYLOAD_LENGTH);

    //check size
    if((currentLength + size) > MAX_PKT_LENGTH){
        size = MAX_PKT_LENGTH - currentLength;
    }

    //write data
    for(int i = 0; i < size; ++i){
        spi_write_reg(REG_FIFO, buffer[i]);
    }

    //update length
    spi_write_reg(REG_PAYLOAD_LENGTH, currentLength + size);

    //put in TX mode
    spi_write_reg(REG_OPMODE, MODE_LONG_RANGE_MODE | MODE_TX);

    //wait for TX done
    while((spi_read_reg(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0);

    // clear IRQ's
    spi_write_reg(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);

    return size;
}

void setup_lora(){
    gpio_write(rstPin, 0);
    delay(100);
    gpio_write(rstPin, 1);
    delay(100);

    uint8_t version = spi_read_reg(REG_VERSION);

    printf("Transceiver version 0x%02X, ", version);
    if(version != SX1276_ID){ 
        puts("Unrecognized transceiver");
        exit(1);
    } else {
        puts("SX1276 detected\n");
    }

    spi_write_reg(REG_OPMODE, MODE_SLEEP);

    // set frequency
    uint64_t frf = ((uint64_t)freq << 19) / 32000000;
    spi_write_reg(REG_FRF_MSB, frf >> 16);
    spi_write_reg(REG_FRF_MID, frf >> 8);
    spi_write_reg(REG_FRF_LSB, frf >> 0);

    //LoRaWAN public sync word
    spi_write_reg(REG_SYNC_WORD, 0x34);

    if(sf == 11 || sf == 12){
        spi_write_reg(REG_MODEM_CONFIG3, 0x0C);
    } else {
        spi_write_reg(REG_MODEM_CONFIG3, 0x04);
    }
    spi_write_reg(REG_MODEM_CONFIG, 0x72);
    spi_write_reg(REG_MODEM_CONFIG2, (sf << 4) | 0x04);

    if(sf == 10 || sf == 11 || sf == 12){
        spi_write_reg(REG_SYMB_TIMEOUT_LSB, 0x05);
    } else {
        spi_write_reg(REG_SYMB_TIMEOUT_LSB, 0x08);
    }
    spi_write_reg(REG_MAX_PAYLOAD_LENGTH, 0x80);
    spi_write_reg(REG_PAYLOAD_LENGTH, PAYLOAD_LENGTH);
    spi_write_reg(REG_HOP_PERIOD, 0xFF);
    spi_write_reg(REG_FIFO_ADDR_PTR, spi_read_reg(REG_FIFO_RX_BASE_AD));

    //set Continous Receive Mode
    spi_write_reg(REG_LNA, LNA_MAX_GAIN); //max lna gain
    spi_write_reg(REG_OPMODE, MODE_RX_CONTINUOUS);
}

void send_stat(){
    //status report packet
    char status_pkt[STATUS_SIZE];

    fill_pkt_header(status_pkt);

    //get timestamp for statistics
    char stat_timestamp[24];
    time_t t = time(NULL);
    strftime(stat_timestamp, sizeof(stat_timestamp), "%F %T %Z", gmtime(&t));

    json_t *root = json_object();
    json_object_set_new(root, "stat", 
            json_pack("{ss,sf,sf,si,si,si,si,sf,si,si,ss,ss,ss}",
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
                "desc", description));  //string

    const char *json_str = json_dumps(root, JSON_COMPACT);
    printf("stat update: %s\n", json_str);

    printf("stat update: %s", stat_timestamp);
    if(cp_nb_rx_ok == 0){
        printf(" no packet received yet\n");
    } else {
        printf(" %u packet%sreceived\n", cp_nb_rx_ok, cp_nb_rx_ok > 1 ? "s " : " ");
    }

    int json_strlen = strlen(json_str);

    //build and send message
    memcpy(status_pkt + HEADER_SIZE, json_str, json_strlen);
    for(int i = 0; i < numservers; ++i){
        send_udp(servers[i], status_pkt, HEADER_SIZE + json_strlen);
    }

    //free json memory
    json_decref(root);
}

void send_ack(const char *message){
    char pkt[ACK_HEADER_SIZE];
    pkt[0] = PROTOCOL_VERSION;
    pkt[1] = message[1];
    pkt[2] = message[2];
    pkt[3] = PKT_PUSH_ACK;

    write_data(pkt, ACK_HEADER_SIZE);
}

bool receive_packet(void){
    wait_irq();

    char message[256];
    uint8_t length;
    if(!read_data(message, &length)){
        return false;
    }

    long int SNR;
    uint8_t value = spi_read_reg(REG_PKT_SNR_VALUE);
    //the SNR sign bit is 1
    if(value & 0x80){
        //invert and divide by 4
        value = ((~value + 1) & 0xFF) >> 2;
        SNR = -value;
    } else {
        // Divide by 4
        SNR = (value & 0xFF) >> 2;
    }

    const int rssicorr = 157;
    int rssi = spi_read_reg(REG_PKT_RSSI) - rssicorr;

    printf("Packet RSSI: %d, ", rssi);
    printf("RSSI: %d, ", spi_read_reg(REG_RSSI) - rssicorr);
    printf("SNR: %li, ", SNR);
    printf("Length: %hhu Message:'", length);
    for(int i = 0; i < length; ++i){
        printf("%c", isprint(message[i]) ? message[i] : '.');
    }
    printf("'\n");

    //buffer to compose the upstream packet
    char pkt[TX_BUFF_SIZE];

    fill_pkt_header(pkt);

    //TODO: start_time can jump if time is (re)set, not good
    struct timeval now;
    gettimeofday(&now, NULL);
    uint32_t start_time = now.tv_sec * 1000000 + now.tv_usec;

    //encode payload
    char b64[BASE64_MAX_LENGTH];
    bin_to_b64((uint8_t*)message, length, b64, BASE64_MAX_LENGTH);

    char datr[] = "SFxxBWxxx";
    snprintf(datr, strlen(datr) + 1, "SF%hhuBW%hu", sf, bw);

    json_t *root = json_object();
    json_t *arr  = json_array();
    json_array_append_new(arr,
            json_pack("{si,sf,si,si,si,ss,ss,ss,si,si,si,ss}",
                "tmst", start_time,           //uint
                "freq", mhz,                  //double
                "chan", 0,                    //uint
                "rfch", 0,                    //uint
                "stat", 1,                    //uint
                "modu", "LORA",               //string
                "datr", datr,                 //string
                "codr", "4/5",                //string
                "rssi", rssi,                 //int
                "lsnr", SNR,                  //long
                "size", length,               //uint
                "data", b64));                //string

    json_object_set_new(root, "rxpk", arr);

    const char *json_str = json_dumps(root, JSON_COMPACT);

    //printf("rxpk update: %s\n", json_str);

    int json_strlen = strlen(json_str);

    // Build and send message.
    memcpy(pkt + 12, json_str, json_strlen);
    for(int i = 0; i < numservers; ++i){
        send_udp(servers[i], pkt, HEADER_SIZE + json_strlen);
    }

    //free json memory
    json_decref(root);
    fflush(stdout);
    return true;
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

void init(void){
    //set up hardware
    setup_interrupt("rising"); //gpio4, input
    rstPin = gpio_init("/sys/class/gpio/gpio3/value", O_WRONLY); //gpio 3, output
    spi_init("/dev/spidev0.0", O_RDWR);

    //setup LoRa
    setup_lora();

    print_configuration();

    prepare_socket();

    printf("Listening at SF%i on %.6lf Mhz.\n", sf, mhz);
    printf("-----------------------------------\n");
}

int main(){
    init();

    uint32_t lasttime = seconds();

    while(1){
        receive_packet();

        int nowseconds = seconds();
        if(nowseconds - lasttime >= 30){
            lasttime = nowseconds;
            send_stat();
            cp_nb_rx_rcv  = 0;
            cp_nb_rx_ok   = 0;
            cp_up_pkt_fwd = 0;
        }
    }
}
