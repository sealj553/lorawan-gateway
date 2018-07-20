#ifndef REGISTERS_H
#define REGISTERS_H

#define REG_FIFO                    0x00
#define REG_FIFO_ADDR_PTR           0x0D
#define REG_FIFO_TX_BASE_AD         0x0E
#define REG_FIFO_RX_BASE_AD         0x0F
#define REG_RX_NB_BYTES             0x13
#define REG_OPMODE                  0x01
#define REG_FIFO_RX_CURRENT_ADDR    0x10
#define REG_IRQ_FLAGS               0x12
#define REG_DIO_MAPPING_1           0x40
#define REG_DIO_MAPPING_2           0x41
#define REG_MODEM_CONFIG            0x1D
#define REG_MODEM_CONFIG2           0x1E
#define REG_MODEM_CONFIG3           0x26
#define REG_SYMB_TIMEOUT_LSB        0x1F
#define REG_PKT_SNR_VALUE           0x19
#define REG_PAYLOAD_LENGTH          0x22
#define REG_IRQ_FLAGS_MASK          0x11
#define REG_MAX_PAYLOAD_LENGTH      0x23
#define REG_HOP_PERIOD              0x24
#define REG_SYNC_WORD               0x39
#define REG_VERSION                 0x42
#define REG_RSSI                    0x1B
#define REG_PKT_RSSI                0x1A

#define SX1276_ID                   0x12

//where do these values even come from? not the datasheet...
//#define SX72_MODE_RX_CONTINUOS      0x85
//#define SX72_MODE_TX                0x83
//#define SX72_MODE_SLEEP             0x80
//#define SX72_MODE_STANDBY           0x81

// modes
#define MODE_LONG_RANGE_MODE        0x80
#define MODE_SLEEP                  0x00
#define MODE_STDBY                  0x01
#define MODE_TX                     0x03
#define MODE_RX_CONTINUOUS          0x05
#define MODE_RX_SINGLE              0x06

#define PAYLOAD_LENGTH              0x40
#define PAYLOAD_CRC                 0x20

// LOW NOISE AMPLIFIER
#define REG_LNA                     0x0C
#define LNA_MAX_GAIN                0x23
#define LNA_OFF_GAIN                0x00
#define LNA_LOW_GAIN                0x20

// CONF REG
#define REG1                        0x0A
#define REG2                        0x84

#define SX72_MC1_LOW_DATA_RATE_OPTIMIZE  0x01 // mandated for SF11 and SF12

// FRF
#define REG_FRF_MSB                 0x06
#define REG_FRF_MID                 0x07
#define REG_FRF_LSB                 0x08

//Max length of buffer
#define BUFLEN                      2048
#define MAX_PKT_LENGTH              255

#define PROTOCOL_VERSION            1
#define PKT_PUSH_DATA               0
#define PKT_PUSH_ACK                1
#define PKT_PULL_DATA               2

#define PKT_PULL_RESP               3
#define PKT_PULL_ACK                4

#define TX_BUFF_SIZE                2048
#define STATUS_SIZE                 1024
#define BASE64_MAX_LENGTH           341
#define HEADER_SIZE                 12
#define ACK_HEADER_SIZE             4

// IRQ masks
#define IRQ_TX_DONE_MASK           0x08
#define IRQ_PAYLOAD_CRC_ERROR_MASK 0x20
#define IRQ_RX_DONE_MASK           0x40

#endif
