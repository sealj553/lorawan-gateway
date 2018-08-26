//TODO: thread for checking irc pin, callback queue

#include "registers.h"
#include "config.h"
#include "base64.h"
#include "spi.h"
#include "gpio.h"
#include "time_util.h"
#include "connector.h"
#include "sx1276.h"

#include <protobuf-c.h>

#include <sys/time.h>

#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

uint32_t rx_rcv  = 0;
uint32_t rx_ok   = 0;

/*uint32_t rx_rcv;
uint32_t rx_ok;
uint32_t rx_bad;
uint32_t rx_nocrc;
uint32_t pkt_fwd;
uint32_t network_byte;
uint32_t payload_byte;
uint32_t dgram_sent;
uint32_t ack_rcv;
uint32_t pull_sent;
uint32_t ack_rcv;
uint32_t dgram_rcv;
uint32_t network_byte;
uint32_t payload_byte;
uint32_t tx_ok;
uint32_t tx_fail;
uint32_t tx_requested = 0;
uint32_t tx_rejected_collision_packet = 0;
uint32_t tx_rejected_collision_beacon = 0;
uint32_t tx_rejected_too_late = 0;
uint32_t tx_rejected_too_early = 0;
uint32_t beacon_queued = 0;
uint32_t beacon_sent = 0;
uint32_t beacon_rejected = 0;*/

//uint32_t cp_up_pkt_fwd = 0;

int irqPin, rstPin, intPin;
const double mhz = (double)freq/1000000;
TTN *ttn;
int running = 1;

//void die(const char *s);
void setup_lora();
void send_status();
void receive_packet(void);
void init(void);
void send_ack(const uint8_t *received_message);
void print_downlink(Router__DownlinkMessage *msg, void *arg);
void reset_radio(void);
void cleanup(void);
long get_snr(void);
int get_rssi(void);
void ttn_connect(void);
void ttn_disconnect(void);

/*void die(const char *s){
  perror(s);
  exit(1);
  }*/

volatile sig_atomic_t pipe_broke = 0;

void sigpipe_handler(int sig){
    pipe_broke = 1;
}

void stop(int sig){
    signal(SIGINT, NULL);
    running = 0;
}

void reset_radio(){
    gpio_write(rstPin, 1);
    delay(200);
    gpio_write(rstPin, 0);
    delay(200);
    gpio_write(rstPin, 1);
    delay(50);
}

/*void printBits(size_t const size, void const * const ptr){
    unsigned char *b = (unsigned char*)ptr;
    unsigned char byte;
    int i, j;

    for(i=size-1;i>=0;i--){
        for(j=7;j>=0;j--){
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("");
}*/

void setup_lora(){
    reset_radio();

    uint8_t version = get_version();
    printf("Transceiver version 0x%02X, ", version);
    if(version != SX1276_ID){ 
        puts("Unrecognized transceiver");
        exit(1);
    }
    puts("SX1276 detected\n");

    set_frequency(freq);
    set_sync_word(LORAWAN_PUBLIC_SYNC_WORD);

    spi_write_reg(REG_MODEM_CONFIG1, MODEM_CONFIG1_VAL);
    spi_write_reg(REG_MODEM_CONFIG2, MODEM_CONFIG2_VAL);
    spi_write_reg(REG_MODEM_CONFIG3, MODEM_CONFIG3_VAL);

#if SF_VAL == 10 | SF_VAL == 11 | SF_VAL == 12
    spi_write_reg(REG_SYMB_TIMEOUT_LSB, 0x05);
#else
    spi_write_reg(REG_SYMB_TIMEOUT_LSB, 0x08);
#endif

    spi_write_reg(REG_MAX_PAYLOAD_LENGTH, PAYLOAD_LENGTH);
    spi_write_reg(REG_PAYLOAD_LENGTH, PAYLOAD_LENGTH);
    spi_write_reg(REG_HOP_PERIOD, 0xFF);
    spi_write_reg(REG_FIFO_ADDR_PTR, spi_read_reg(REG_FIFO_RX_BASE_ADDR));

    //set base addresses
    spi_write_reg(REG_FIFO_TX_BASE_ADDR, 0);
    spi_write_reg(REG_FIFO_RX_BASE_ADDR, 0);

    //set Continous Receive Mode
    spi_write_reg(REG_LNA, LNA_MAX_GAIN);
    spi_write_reg(REG_OPMODE, MODE_RX_CONTINUOUS);
}

void print_downlink(Router__DownlinkMessage *msg, void *arg){
    //if(!msg->payload.len){ return; }
    printf("down: have %zu bytes downlink\n", msg->payload.len);
    if(msg->protocol_configuration->protocol_case == PROTOCOL__TX_CONFIGURATION__PROTOCOL_LORAWAN){
        Lorawan__TxConfiguration *lora = msg->protocol_configuration->lorawan;
        //printf("down: modulation: %d, data rate: %s, bit rate: %d, coding rate: "
        //"%s, fcnt: %d\n",
        //lora->modulation, lora->data_rate, lora->bit_rate,
        //lora->coding_rate, lora->f_cnt);
        Gateway__TxConfiguration *gtw = msg->gateway_configuration;
        printf("down: timestamp: %d, rf chain: %d, frequency: %lu, power: %d, "
                "polarization inversion: %d, frequency deviation: %d\n",
                gtw->timestamp, gtw->rf_chain, gtw->frequency, gtw->power,
                gtw->polarization_inversion, gtw->frequency_deviation);
    } else {
        printf("down: invalid protocol %d\n", msg->protocol_configuration->protocol_case);
    }
}

void send_status(){
    //char stat_timestamp[24];
    //time_t t = time(NULL);
    //strftime(stat_timestamp, sizeof(stat_timestamp), "%F %T %Z", gmtime(&t));

    //printf("stat update: %s", stat_timestamp);
    printf("%u packets received\n", rx_ok);

    // Send gateway status
    Gateway__Status status  = GATEWAY__STATUS__INIT;
    status.time             = get_time_ns();
    //status.gateway_trusted
    status.boot_time        = get_boot_time_ns();
    //status.ip
    status.platform         = platform;
    status.contact_email    = email;
    status.description      = description;
    status.frequency_plan   = frequency_plan;

    Gateway__LocationMetadata location = GATEWAY__LOCATION_METADATA__INIT;
    location.latitude       = lat;
    location.longitude      = lon;
    location.altitude       = alt;
    location.accuracy       = 10;
    status.location = &location;

    //status.location
    status.rx_in            = rx_rcv;
    status.rx_ok            = rx_ok;
    //status.tx_in
    //status.tx_ok
    //status.lm_ok
    //status.lm_st
    //status.lm_nw
    //status.l_pps
    Gateway__Status__OSMetrics os = GATEWAY__STATUS__OSMETRICS__INIT;
    os.cpu_percentage       = 0;
    os.memory_percentage    = 0;
    os.temperature          = 999;
    status.os = &os;

    int err;
send_status:
    err = ttngwc_send_status(ttn, &status);
    if(err){
        printf("send failed: %d\n", err);
        puts("retrying...");
        ttn_disconnect();
        ttn_connect();
        delay(1000);
        if(!running){
            return;    
        }
        goto send_status;
    } else {
        puts("status sent");
    }
}

void send_ack(const uint8_t *received_message){
    char pkt[ACK_HEADER_SIZE];
    pkt[0] = PROTOCOL_VERSION;
    pkt[1] = received_message[1];
    pkt[2] = received_message[2];
    pkt[3] = PKT_PUSH_ACK;

    write_data(pkt, ACK_HEADER_SIZE);
}

long get_snr(void){
    long SNR;
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
    return SNR;
}

int get_rssi(void){
    const int rssicorr = 157;
    return spi_read_reg(REG_PKT_RSSI) - rssicorr;
}

void receive_packet(void){
    //wait_irq();
    if(!gpio_read(irqPin)){
        return;
    }

    ++rx_rcv;

    puts("dio0 rise, packet received?");

    uint8_t message[MAX_MESSAGE_SIZE];
    uint8_t length;
    if(!read_data(message, &length)){
        puts("can't read data");
        return;
    }
    puts("got packet");

    ++rx_ok;
    //if confirmed
    send_ack(message);
    puts("ack sent\n");

    ///// fake data
    message[0] = 'h';
    message[1] = 'e';
    message[2] = 'l';
    message[3] = 'l';
    message[4] = 'o';

    /////

    long SNR = get_snr();
    int rssi = get_rssi();

    printf("Packet RSSI: %d, ", rssi);
    printf("RSSI: %d, ", rssi);
    printf("SNR: %li, ", SNR);
    printf("Length: %hhu Message:'", length);
    for(int i = 0; i < length; ++i){
        printf("%c", isprint(message[i]) ? message[i] : '.');
    }
    printf("'\n");

    //uint32_t start_time = get_time();

    //encode payload
    //char b64[BASE64_MAX_LENGTH];
    //bin_to_b64(message, length, b64, BASE64_MAX_LENGTH);

    //char datr[] = "SFxxBWxxx";
    //snprintf(datr, strlen(datr) + 1, "SF%hhuBW%hu", sf, bw);

    // Enter the payload
    //unsigned char buf[] = {0x1, 0x2, 0x3, 0x4, 0x5};
    //uint8_t buf[] = "hello world";
    Router__UplinkMessage up        = ROUTER__UPLINK_MESSAGE__INIT;
    up.payload.len                  = length;
    up.payload.data                 = message;

    // Set protocol metadata
    Protocol__RxMetadata protocol   = PROTOCOL__RX_METADATA__INIT;
    protocol.protocol_case          = PROTOCOL__RX_METADATA__PROTOCOL_LORAWAN;
    Lorawan__Metadata lorawan       = LORAWAN__METADATA__INIT;
    lorawan.modulation              = LORAWAN__MODULATION__LORA;
    lorawan.data_rate               = data_rate;
    lorawan.coding_rate             = "4/5";
    lorawan.f_cnt                   = 0; //frame count

    protocol.lorawan     = &lorawan;
    up.protocol_metadata = &protocol;

    // Set gateway metadata
    Gateway__RxMetadata gateway     = GATEWAY__RX_METADATA__INIT;

    gateway.timestamp               = uptime_us(); //Timestamp (uptime of LoRa module) in microseconds with rollover

    //RF chain where the gateway received the message
    gateway.rf_chain                = 0;
    gateway.frequency               = freq;
    up.gateway_metadata = &gateway;

    // Send uplink message
    int err;
send_uplink:
    err = ttngwc_send_uplink(ttn, &up);
    if(err){
        printf("up: send failed: %d\n", err);
        if(pipe_broke){
            puts("retrying...");
            ttn_disconnect();
            delay(1000);
            ttn_connect();
            delay(500);
            if(!running){
                return;
            }
            goto send_uplink;
        }
    } else {
        puts("uplink sent");
    }
}

void init(void){
    boot_time();
    //set up hardware
    ////setup_interrupt("rising"); //gpio4, input
    irqPin = gpio_init("/sys/class/gpio/gpio4/value", O_RDONLY);//gpio 4, input
    rstPin = gpio_init("/sys/class/gpio/gpio3/value", O_WRONLY);//gpio 3, output
    spi_init("/dev/spidev0.0", O_RDWR);

    //setup LoRa
    setup_lora();

    //sigint handler
    signal(SIGINT, stop);
    signal(SIGTERM, stop);
    //sigaction handler
    struct sigaction handler;
    memset(&handler, 0, sizeof(struct sigaction));
    handler.sa_handler = sigpipe_handler;
    sigaction(SIGPIPE, &handler, NULL);

    // Initialize the TTN gateway
    ttngwc_init(&ttn, gateway_id, &print_downlink, NULL);
    if(!ttn){
        printf("failed to initialize TTN gateway\n");
        return;
    }

    ttn_connect();

    printf("Listening at SF%i on %.6lf Mhz.\n", SF_VAL, mhz);
    printf("-----------------------------------\n");
}

void ttn_connect(void){
    // Connect to the broker
    printf("connecting...\n");
    int err = ttngwc_connect(ttn, server_hostname, server_port, gateway_key);
    if(err != 0){
        printf("connect failed: %d\n", err);
        ttn_disconnect();
        delay(500);
        if(running){
        ttn_connect();
        delay(500);
        } else {
            return;
        }
        //exit(0);
    }
    printf("connected\n");
}

void ttn_disconnect(void){
    ttngwc_disconnect(ttn);
}

void cleanup(void){
    ttngwc_cleanup(ttn);
    ttngwc_disconnect(ttn);
    puts("shutting down...");
}

int main(){
    init();
    send_status();

    uint32_t lasttime = seconds();
    while(running){
        receive_packet();

        int nowseconds = seconds();
        if(nowseconds - lasttime >= update_interval){
            send_status();
        //    rx_rcv  = 0;
        //    rx_ok   = 0;
        //    cp_up_pkt_fwd = 0;
            lasttime = nowseconds;
        }
    }
    cleanup();
}
