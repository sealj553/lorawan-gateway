#include "registers.h"
#include "config.h"
#include "base64.h"
#include "spi.h"
#include "gpio.h"
#include "time_util.h"
#include "connector.h"

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

//void die(const char *s);
bool read_data(uint8_t *payload, uint8_t *p_length);
void setup_lora();
void send_status();
void receive_packet(void);
void init(void);
void send_ack(const uint8_t *message);
void print_downlink(Router__DownlinkMessage *msg, void *arg);
void cleanup(void);

/*void die(const char *s){
  perror(s);
  exit(1);
  }*/

int running = 1;

void stop(int sig){
    signal(SIGINT, NULL);
    running = 0;
}

bool read_data(uint8_t *payload, uint8_t *p_length){
    //clear rxDone
    spi_write_reg(REG_IRQ_FLAGS, PAYLOAD_LENGTH);

    int irqflags = spi_read_reg(REG_IRQ_FLAGS);
    ++rx_rcv;

    if((irqflags & PAYLOAD_CRC) == PAYLOAD_CRC) {
        puts("CRC error");
        spi_write_reg(REG_IRQ_FLAGS, PAYLOAD_CRC);
        return false;
    }

    ++rx_ok;

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
    spi_write_reg(REG_LNA, LNA_MAX_GAIN);
    spi_write_reg(REG_OPMODE, MODE_RX_CONTINUOUS);
}

void print_downlink(Router__DownlinkMessage *msg, void *arg){
    //if(!msg->payload.len){ return; }
    printf("down: have %zu bytes downlink\n", msg->payload.len);
    if(msg->protocol_configuration->protocol_case == PROTOCOL__TX_CONFIGURATION__PROTOCOL_LORAWAN){
        Lorawan__TxConfiguration *lora = msg->protocol_configuration->lorawan;
        printf("down: modulation: %d, data rate: %s, bit rate: %d, coding rate: "
                "%s, fcnt: %d\n",
                lora->modulation, lora->data_rate, lora->bit_rate,
                lora->coding_rate, lora->f_cnt);
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
    if(rx_ok == 0){
        printf(" no packet received yet\n");
    } else {
        printf(" %u packet%sreceived\n", rx_ok, rx_ok > 1 ? "s " : " ");
    }

    //int json_strlen = strlen(json_str);
    //build and send message
    //memcpy(status_pkt + HEADER_SIZE, json_str, json_strlen);
    //send_udp(servers[i], status_pkt, HEADER_SIZE + json_strlen);
    //free json memory
    //json_decref(root);


    // Send gateway status
    Gateway__Status status  = GATEWAY__STATUS__INIT;
    status.time             = 555;
    //status.gateway_trusted
    //status.boot_time
    //status.ip
    status.platform         = platform;
    status.contact_email    = email;
    status.description      = description;
    status.frequency_plan   = frequency_plan;
    //status.location
    //status.rx_in
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

    int err = ttngwc_send_status(ttn, &status);
    if(err){
        printf("status: send failed: %d\n", err);
    } else {
        printf("status: sent with time %d\n", 555);
    }
}

void send_ack(const uint8_t *message){
    char pkt[ACK_HEADER_SIZE];
    pkt[0] = PROTOCOL_VERSION;
    pkt[1] = message[1];
    pkt[2] = message[2];
    pkt[3] = PKT_PUSH_ACK;

    write_data(pkt, ACK_HEADER_SIZE);
}

void receive_packet(void){
    //wait_irq();
    if(!gpio_read(irqPin)){
        return;
    }

    uint8_t message[MAX_MESSAGE_SIZE];
    uint8_t length;
    if(!read_data(message, &length)){
        return;
    }
    //if confirmed
    send_ack(message);
    puts("ack sent\n");

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

    //uint32_t start_time = get_time();

    //encode payload
    char b64[BASE64_MAX_LENGTH];
    bin_to_b64(message, length, b64, BASE64_MAX_LENGTH);

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
    lorawan.coding_rate             = coding_rate;
    lorawan.f_cnt                   = 0; //frame count
    protocol.lorawan     = &lorawan;
    up.protocol_metadata = &protocol;

    // Set gateway metadata
    Gateway__RxMetadata gateway     = GATEWAY__RX_METADATA__INIT;
    gateway.timestamp               = 555;
    //RF chain where the gateway received the message
    gateway.rf_chain                = 0;
    gateway.frequency               = freq;
    up.gateway_metadata = &gateway;

    // Send uplink message
    int err = ttngwc_send_uplink(ttn, &up);
    if(err){
        printf("up: send failed: %d\n", err);
    } else {
        printf("up: sent with timestamp %d\n", 555);
    }
}

void init(void){
    //set up hardware
    ////setup_interrupt("rising"); //gpio4, input
    irqPin = gpio_init("/sys/class/gpio/gpio4/value", O_RDONLY);//gpio 4, input
    rstPin = gpio_init("/sys/class/gpio/gpio3/value", O_WRONLY);//gpio 3, output
    spi_init("/dev/spidev0.0", O_RDWR);

    //setup LoRa
    setup_lora();

    signal(SIGINT, stop);
    signal(SIGTERM, stop);

    // Initialize the TTN gateway
    ttngwc_init(&ttn, gateway_id, &print_downlink, NULL);
    if(!ttn){
        printf("failed to initialize TTN gateway\n");
        return;
    }

    // Connect to the broker
    printf("connecting...\n");
    int err = ttngwc_connect(ttn, server_hostname, server_port, gateway_key);
    if(err != 0){
        printf("connect failed: %d\n", err);
        ttngwc_cleanup(ttn);
        exit(0);
    }
    printf("connected\n");

    printf("Listening at SF%i on %.6lf Mhz.\n", sf, mhz);
    printf("-----------------------------------\n");
}

void cleanup(void){
    ttngwc_cleanup(ttn);
    ttngwc_disconnect(ttn);
    puts("\ndisconnecting...");
}

int main(){
    init();
    send_status();

    uint32_t lasttime = seconds();
    while(running){
        receive_packet();

        //int nowseconds = seconds();
        //if(nowseconds - lasttime >= update_interval){
        //    lasttime = nowseconds;
        //    send_status();
        //    rx_rcv  = 0;
        //    rx_ok   = 0;
        //    cp_up_pkt_fwd = 0;
        //}
    }
    cleanup();
}
