/******************************************************************************
 *
 * Copyright (c) 2015 Thomas Telkamp
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 *******************************************************************************/
#include "registers.h"
 
extern "C" {
#include "base64.h"
#include "spi.h"

#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netdb.h>
}

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

using std::string;
using std::vector;

using namespace rapidjson;

#define BASE64_MAX_LENGTH 341

bool sx1272 = false;

struct sockaddr_in si_other;
int s;
int slen = sizeof(si_other);
struct ifreq ifr;

uint32_t cp_nb_rx_rcv;
uint32_t cp_nb_rx_ok;
uint32_t cp_nb_rx_ok_tot;
uint32_t cp_nb_rx_bad;
uint32_t cp_nb_rx_nocrc;
uint32_t cp_up_pkt_fwd;

enum SpreadingFactor {
    SF7 = 7,
    SF8,
    SF9,
    SF10,
    SF11,
    SF12
};

struct Server {
    string address;
    uint16_t port;
    bool enabled;
};

/*******************************************************************************
 *
 * Default values, configure them in global_conf.json
 *
 *******************************************************************************/
const char *interface = "eth0";

// Set location in global_conf.json
float lat = 0;
float lon = 0;
int   alt = 0;

/* Informal status fields */
char platform[24] ;    /* platform definition */
char email[40] ;       /* used for contact email */
char description[64] ; /* used for free form description */

// Set spreading factor (SF7 - SF12), &nd  center frequency
// Overwritten by the ones set in global_conf.json
SpreadingFactor sf = SF7;
uint16_t bw = 125;
uint32_t freq = 916800000;

// Servers
vector<Server> servers;

// #############################################

void LoadConfiguration(string filename);
void PrintConfiguration();

long millis(void){
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_nsec / 1000000L;
}

void delay(unsigned int ms){
    static struct timespec time;
    time.tv_sec = ms / 1000;
    if(time.tv_sec > 0){
        ms -= time.tv_sec * 1000;
    }
    time.tv_nsec = ms * 1000000L;
    nanosleep(&time, NULL);
}

void Die(const char *s){
    perror(s);
    exit(1);
}

bool ReceivePkt(char* payload, uint8_t* p_length){
    // clear rxDone
    WriteRegister(REG_IRQ_FLAGS, 0x40);

    int irqflags = ReadRegister(REG_IRQ_FLAGS);

    cp_nb_rx_rcv++;

    //  payload crc: 0x20
    if((irqflags & 0x20) == 0x20) {
        printf("CRC error\n");
        WriteRegister(REG_IRQ_FLAGS, 0x20);
        return false;

    } else {
        cp_nb_rx_ok++;
        cp_nb_rx_ok_tot++;

        uint8_t currentAddr = ReadRegister(REG_FIFO_RX_CURRENT_ADDR);
        uint8_t receivedCount = ReadRegister(REG_RX_NB_BYTES);
        *p_length = receivedCount;

        WriteRegister(REG_FIFO_ADDR_PTR, currentAddr);

        for(int i = 0; i < receivedCount; i++) {
            payload[i] = ReadRegister(REG_FIFO);
        }
    }
    return true;
}

void SetupLoRa(){
    //char buff[16];
    //printf("Trying to detect module with ");
    //printf("NSS=%s "  , PinName(ssPin, buff));
    //printf("DIO0=%s " , PinName(dio0 , buff));
    //printf("Reset=%s ", PinName(RST  , buff));
    //printf("Led1=%s\n", PinName(Led1 , buff));

    // check basic 
    //if (ssPin == 0xff || dio0 == 0xff) {
    //  Die("Bad pin configuration ssPin and dio0 need at least to be defined");
    //}

    //digitalWrite(RST, HIGH);
    //delay(100);
    //digitalWrite(RST, LOW);
    //delay(100);

    uint8_t version = ReadRegister(REG_VERSION);

    if (version == 0x22) {
        // sx1272
        printf("SX1272 detected, starting.\n");
        sx1272 = true;
    } else {
        // sx1276?
        //digitalWrite(RST, LOW);
        version = ReadRegister(REG_VERSION);
        if (version == 0x12) {
            // sx1276
            printf("SX1276 detected, starting.\n");
            sx1272 = false;
        } else {
            printf("Transceiver version 0x%02X\n", version);
            Die("Unrecognized transceiver");
        }
    }

    WriteRegister(REG_OPMODE, SX72_MODE_SLEEP);

    // set frequency
    uint64_t frf = ((uint64_t)freq << 19) / 32000000;
    WriteRegister(REG_FRF_MSB, (uint8_t)(frf >> 16));
    WriteRegister(REG_FRF_MID, (uint8_t)(frf >> 8));
    WriteRegister(REG_FRF_LSB, (uint8_t)(frf >> 0));

    WriteRegister(REG_SYNC_WORD, 0x34); // LoRaWAN public sync word

    if (sx1272) {
        if (sf == SF11 || sf == SF12) {
            WriteRegister(REG_MODEM_CONFIG, 0x0B);
        } else {
            WriteRegister(REG_MODEM_CONFIG, 0x0A);
        }
        WriteRegister(REG_MODEM_CONFIG2, (sf << 4) | 0x04);
    } else {
        if (sf == SF11 || sf == SF12) {
            WriteRegister(REG_MODEM_CONFIG3, 0x0C);
        } else {
            WriteRegister(REG_MODEM_CONFIG3, 0x04);
        }
        WriteRegister(REG_MODEM_CONFIG, 0x72);
        WriteRegister(REG_MODEM_CONFIG2, (sf << 4) | 0x04);
    }

    if (sf == SF10 || sf == SF11 || sf == SF12) {
        WriteRegister(REG_SYMB_TIMEOUT_LSB, 0x05);
    } else {
        WriteRegister(REG_SYMB_TIMEOUT_LSB, 0x08);
    }
    WriteRegister(REG_MAX_PAYLOAD_LENGTH, 0x80);
    WriteRegister(REG_PAYLOAD_LENGTH, PAYLOAD_LENGTH);
    WriteRegister(REG_HOP_PERIOD, 0xFF);
    WriteRegister(REG_FIFO_ADDR_PTR, ReadRegister(REG_FIFO_RX_BASE_AD));

    // Set Continous Receive Mode
    WriteRegister(REG_LNA, LNA_MAX_GAIN);  // max lna gain
    WriteRegister(REG_OPMODE, SX72_MODE_RX_CONTINUOS);
}

void SolveHostname(const char* p_hostname, uint16_t port, struct sockaddr_in* p_sin){
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    char service[6] = { '\0' };
    snprintf(service, 6, "%hu", port);

    struct addrinfo* p_result = NULL;

    // Resolve the domain name into a list of addresses
    int error = getaddrinfo(p_hostname, service, &hints, &p_result);
    if (error != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        exit(EXIT_FAILURE);
    }

    // Loop over all returned results
    for (struct addrinfo* p_rp = p_result; p_rp != NULL; p_rp = p_rp->ai_next) {
        struct sockaddr_in* p_saddr = (struct sockaddr_in*)p_rp->ai_addr;
        //printf("%s solved to %s\n", p_hostname, inet_ntoa(p_saddr->sin_addr));
        p_sin->sin_addr = p_saddr->sin_addr;
    }

    freeaddrinfo(p_result);
}

void SendUdp(char *msg, int length){
    for(vector<Server>::iterator it = servers.begin(); it != servers.end(); ++it){
        if(it->enabled){
            si_other.sin_port = htons(it->port);

            SolveHostname(it->address.c_str(), it->port, &si_other);
            if(sendto(s, (char *)msg, length, 0, (struct sockaddr*) &si_other, slen) == -1){
                Die("sendto()");
            }
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
    status_report[4]  = (unsigned char)ifr.ifr_hwaddr.sa_data[0];
    status_report[5]  = (unsigned char)ifr.ifr_hwaddr.sa_data[1];
    status_report[6]  = (unsigned char)ifr.ifr_hwaddr.sa_data[2];
    status_report[7]  = 0xFF;
    status_report[8]  = 0xFF;
    status_report[9]  = (unsigned char)ifr.ifr_hwaddr.sa_data[3];
    status_report[10] = (unsigned char)ifr.ifr_hwaddr.sa_data[4];
    status_report[11] = (unsigned char)ifr.ifr_hwaddr.sa_data[5];

    /* start composing datagram with the header */
    uint8_t token_h = (uint8_t)rand(); /* random token */
    uint8_t token_l = (uint8_t)rand(); /* random token */
    status_report[1] = token_h;
    status_report[2] = token_l;
    stat_index = 12; /* 12-byte header */

    /* get timestamp for statistics */
    time_t t = time(NULL);
    strftime(stat_timestamp, sizeof stat_timestamp, "%F %T %Z", gmtime(&t));

    // Build JSON object.
    StringBuffer sb;
    Writer<StringBuffer> writer(sb);
    writer.StartObject();
    writer.String("stat");
    writer.StartObject();
    writer.String("time");
    writer.String(stat_timestamp);
    writer.String("lati");
    writer.Double(lat);
    writer.String("long");
    writer.Double(lon);
    writer.String("alti");
    writer.Int(alt);
    writer.String("rxnb");
    writer.Uint(cp_nb_rx_rcv);
    writer.String("rxok");
    writer.Uint(cp_nb_rx_ok);
    writer.String("rxfw");
    writer.Uint(cp_up_pkt_fwd);
    writer.String("ackr");
    writer.Double(0);
    writer.String("dwnb");
    writer.Uint(0);
    writer.String("txnb");
    writer.Uint(0);
    writer.String("pfrm");
    writer.String(platform);
    writer.String("mail");
    writer.String(email);
    writer.String("desc");
    writer.String(description);
    writer.EndObject();
    writer.EndObject();

    string json = sb.GetString();
    //printf("stat update: %s\n", json.c_str());
    printf("stat update: %s", stat_timestamp);
    if (cp_nb_rx_ok_tot==0) {
        printf(" no packet received yet\n");
    } else {
        printf(" %u packet%sreceived\n", cp_nb_rx_ok_tot, cp_nb_rx_ok_tot>1?"s ":" ");
    }

    // Build and send message.
    memcpy(status_report + 12, json.c_str(), json.size());
    SendUdp(status_report, stat_index + json.size());
}

bool Receivepacket(){
    long int SNR;
    int rssicorr;
    bool ret = false;

    //if (digitalRead(dio0) == 1) {
    //fix!
    if(1){
        char message[256];
        uint8_t length = 0;
        if (ReceivePkt(message, &length)) {
            // OK got one
            ret = true;

            uint8_t value = ReadRegister(REG_PKT_SNR_VALUE);
            if (value & 0x80) { // The SNR sign bit is 1
                // Invert and divide by 4
                value = ((~value + 1) & 0xFF) >> 2;
                SNR = -value;
            } else {
                // Divide by 4
                SNR = ( value & 0xFF ) >> 2;
            }

            rssicorr = sx1272 ? 139 : 157;

            printf("Packet RSSI: %d, ", ReadRegister(0x1A) - rssicorr);
            printf("RSSI: %d, ", ReadRegister(0x1B) - rssicorr);
            printf("SNR: %li, ", SNR);
            printf("Length: %hhu Message:'", length);
            for (int i=0; i<length; i++) {
                char c = (char) message[i];
                printf("%c",isprint(c)?c:'.');
            }
            printf("'\n");

            char buff_up[TX_BUFF_SIZE]; /* buffer to compose the upstream packet */
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

            buff_up[4] = (uint8_t)ifr.ifr_hwaddr.sa_data[0];
            buff_up[5] = (uint8_t)ifr.ifr_hwaddr.sa_data[1];
            buff_up[6] = (uint8_t)ifr.ifr_hwaddr.sa_data[2]; 
            buff_up[7] = 0xFF;
            buff_up[8] = 0xFF;
            buff_up[9] = (uint8_t)ifr.ifr_hwaddr.sa_data[3];
            buff_up[10] = (uint8_t)ifr.ifr_hwaddr.sa_data[4];
            buff_up[11] = (uint8_t)ifr.ifr_hwaddr.sa_data[5];

            /* start composing datagram with the header */
            uint8_t token_h = (uint8_t)rand(); /* random token */
            uint8_t token_l = (uint8_t)rand(); /* random token */
            buff_up[1] = token_h;
            buff_up[2] = token_l;
            buff_index = 12; /* 12-byte header */

            // TODO: tmst can jump is time is (re)set, not good.
            struct timeval now;
            gettimeofday(&now, NULL);
            uint32_t tmst = (uint32_t)(now.tv_sec * 1000000 + now.tv_usec);

            // Encode payload.
            char b64[BASE64_MAX_LENGTH];
            bin_to_b64((uint8_t*)message, length, b64, BASE64_MAX_LENGTH);

            // Build JSON object.
            StringBuffer sb;
            Writer<StringBuffer> writer(sb);
            writer.StartObject();
            writer.String("rxpk");
            writer.StartArray();
            writer.StartObject();
            writer.String("tmst");
            writer.Uint(tmst);
            writer.String("freq");
            writer.Double((double)freq / 1000000);
            writer.String("chan");
            writer.Uint(0);
            writer.String("rfch");
            writer.Uint(0);
            writer.String("stat");
            writer.Uint(1);
            writer.String("modu");
            writer.String("LORA");
            writer.String("datr");
            char datr[] = "SFxxBWxxx";
            snprintf(datr, strlen(datr) + 1, "SF%hhuBW%hu", sf, bw);
            writer.String(datr);
            writer.String("codr");
            writer.String("4/5");
            writer.String("rssi");
            writer.Int(ReadRegister(0x1A) - rssicorr);
            writer.String("lsnr");
            writer.Double(SNR); // %li.
            writer.String("size");
            writer.Uint(length);
            writer.String("data");
            writer.String(b64);
            writer.EndObject();
            writer.EndArray();
            writer.EndObject();

            string json = sb.GetString();
            printf("rxpk update: %s\n", json.c_str());

            // Build and send message.
            memcpy(buff_up + 12, json.c_str(), json.size());
            SendUdp(buff_up, buff_index + json.size());

            fflush(stdout);
        }
    }
    return ret;
}

int main(){
    struct timeval nowtime;
    uint32_t lasttime;
    //unsigned int led1_timer;

    LoadConfiguration("global_conf.json");
    PrintConfiguration();

    // Init WiringPI
    //wiringPiSetup() ;
    //pinMode(ssPin, OUTPUT);
    //pinMode(dio0, INPUT);
    //pinMode(RST, OUTPUT);

    if(!spi_init()){
        fprintf(stderr, "Failed to initialize SPI: %s\n", spi_get_error());
        exit(1);
    }

    // LED ?
    //if (Led1 != 0xff) {
    //  pinMode(Led1, OUTPUT);

    //  // Blink to indicate startup
    //  for (uint8_t i=0; i<5 ; i++) {
    //    digitalWrite(Led1, 1);
    //    delay(200);
    //    digitalWrite(Led1, 0);
    //    delay(200);
    //  }
    //}

    // Init SPI
    //fix
    //wiringPiSPISetup(SPI_CHANNEL, 500000);

    // Setup LORA
    SetupLoRa();

    // Prepare Socket connection
    if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        Die("socket");
    }

    memset((char*)&si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    ioctl(s, SIOCGIFHWADDR, &ifr);

    // ID based on MAC Adddress of eth0
    printf("Gateway ID: %.2x:%.2x:%.2x:ff:ff:%.2x:%.2x:%.2x\n",
            (uint8_t)ifr.ifr_hwaddr.sa_data[0],
            (uint8_t)ifr.ifr_hwaddr.sa_data[1],
            (uint8_t)ifr.ifr_hwaddr.sa_data[2],
            (uint8_t)ifr.ifr_hwaddr.sa_data[3],
            (uint8_t)ifr.ifr_hwaddr.sa_data[4],
            (uint8_t)ifr.ifr_hwaddr.sa_data[5]
          );

    printf("Listening at SF%i on %.6lf Mhz.\n", sf,(double)freq/1000000);
    printf("-----------------------------------\n");

    while(1) {
        // Packet received ?
        if(Receivepacket()){
            printf("Packet received!\n");
            // Led ON
            //if (Led1 != 0xff) {
            //  digitalWrite(Led1, 1);
            //}
            // start our Led blink timer, LED as been lit in Receivepacket
            //fix
            //led1_timer = millis();
        }

        gettimeofday(&nowtime, NULL);
        uint32_t nowseconds = (uint32_t)(nowtime.tv_sec);
        if (nowseconds - lasttime >= 30) {
            lasttime = nowseconds;
            SendStat();
            cp_nb_rx_rcv = 0;
            cp_nb_rx_ok = 0;
            cp_up_pkt_fwd = 0;
        }

        // Led timer in progress ?
        /*if (led1_timer) {
            // Led timer expiration, Blink duration is 250ms
            if (millis() - led1_timer >= 250) {
                // Stop Led timer
                led1_timer = 0;

                // Led OFF
                if (Led1 != 0xff) {
                    digitalWrite(Led1, 0);
                }
            }
        }*/

        // Let some time to the OS
        delay(1);
    }

    return 0;
}

void LoadConfiguration(string configurationFile){
    FILE* p_file = fopen(configurationFile.c_str(), "r");
    char buffer[65536];
    FileReadStream fs(p_file, buffer, sizeof(buffer));

    Document document;
    document.ParseStream(fs);

    for(Value::ConstMemberIterator fileIt = document.MemberBegin(); fileIt != document.MemberEnd(); ++fileIt){
        string objectType(fileIt->name.GetString());
        if (objectType.compare("SX127x_conf") == 0){
            const Value& sx127x_conf = fileIt->value;
            if(sx127x_conf.IsObject()){
                for(Value::ConstMemberIterator confIt = sx127x_conf.MemberBegin(); confIt != sx127x_conf.MemberEnd(); ++confIt){
                    string key(confIt->name.GetString());
                    if(key.compare("freq") == 0){
                        freq = confIt->value.GetUint();
                    } else if(key.compare("spread_factor") == 0){
                        sf = (SpreadingFactor)confIt->value.GetUint();
                    }
                }
            }

        } else if(objectType.compare("gateway_conf") == 0){
            const Value& gateway_conf = fileIt->value;
            if(gateway_conf.IsObject()){
                for(Value::ConstMemberIterator confIt = gateway_conf.MemberBegin(); confIt != gateway_conf.MemberEnd(); ++confIt){
                    string memberType(confIt->name.GetString());
                    if(memberType.compare("ref_latitude") == 0){
                        lat = confIt->value.GetDouble();
                    } else if(memberType.compare("ref_longitude") == 0){
                        lon = confIt->value.GetDouble();
                    } else if(memberType.compare("ref_altitude") == 0){
                        alt = confIt->value.GetUint(); 

                    } else if(memberType.compare("name") == 0 && confIt->value.IsString()){
                        string str = confIt->value.GetString();
                        strcpy(platform, str.length()<=24 ? str.c_str() : "name too long");
                    } else if(memberType.compare("email") == 0 && confIt->value.IsString()){
                        string str = confIt->value.GetString();
                        strcpy(email, str.length()<=40 ? str.c_str() : "email too long");
                    } else if(memberType.compare("desc") == 0 && confIt->value.IsString()){
                        string str = confIt->value.GetString();
                        strcpy(description, str.length()<=64 ? str.c_str() : "description is too long");

                    } else if(memberType.compare("servers") == 0){
                        const Value& serverConf = confIt->value;
                        if(serverConf.IsObject()){
                            const Value& serverValue = serverConf;
                            Server server;
                            for(Value::ConstMemberIterator srvIt = serverValue.MemberBegin(); srvIt != serverValue.MemberEnd(); ++srvIt){
                                string key(srvIt->name.GetString());
                                if(key.compare("address") == 0 && srvIt->value.IsString()){
                                    server.address = srvIt->value.GetString();
                                } else if(key.compare("port") == 0 && srvIt->value.IsUint()){
                                    server.port = srvIt->value.GetUint();
                                } else if(key.compare("enabled") == 0 && srvIt->value.IsBool()){
                                    server.enabled = srvIt->value.GetBool();
                                }
                            }
                            servers.push_back(server);
                        } else if(serverConf.IsArray()){
                            for(SizeType i = 0; i < serverConf.Size(); ++i){
                                const Value& serverValue = serverConf[i];
                                Server server;
                                for(Value::ConstMemberIterator srvIt = serverValue.MemberBegin(); srvIt != serverValue.MemberEnd(); ++srvIt){
                                    string key(srvIt->name.GetString());
                                    if(key.compare("address") == 0 && srvIt->value.IsString()){
                                        server.address = srvIt->value.GetString();
                                    } else if(key.compare("port") == 0 && srvIt->value.IsUint()){
                                        server.port = srvIt->value.GetUint();
                                    } else if(key.compare("enabled") == 0 && srvIt->value.IsBool()){
                                        server.enabled = srvIt->value.GetBool();
                                    }
                                }
                                servers.push_back(server);
                            }
                        }
                    }
                }
            }
        }
    }
}

void PrintConfiguration(){
    for(vector<Server>::iterator it = servers.begin(); it != servers.end(); ++it){
        printf("server: .address = %s; .port = %hu; .enable = %d\n", it->address.c_str(), it->port, it->enabled);
    }
    printf("Gateway Configuration\n");
    printf("  %s (%s)\n  %s\n", platform, email, description);
    printf("  Latitude=%.8f\n  Longitude=%.8f\n  Altitude=%d\n", lat,lon,alt);
}
