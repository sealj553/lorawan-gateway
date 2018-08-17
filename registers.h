#ifndef REGISTERS_H
#define REGISTERS_H

#define REG_FIFO                    0x00
#define REG_FIFO_ADDR_PTR           0x0D
#define REG_FIFO_TX_BASE_ADDR       0x0E
#define REG_FIFO_RX_BASE_ADDR       0x0F
#define REG_RX_NB_BYTES             0x13
#define REG_OPMODE                  0x01
#define REG_FIFO_RX_CURRENT_ADDR    0x10
#define REG_IRQ_FLAGS               0x12
#define REG_DIO_MAPPING_1           0x40
#define REG_DIO_MAPPING_2           0x41
#define REG_MODEM_CONFIG1           0x1D
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
#define MAX_MESSAGE_SIZE            256

// IRQ masks
#define IRQ_TX_DONE_MASK           0x08
#define IRQ_PAYLOAD_CRC_ERROR_MASK 0x20
#define IRQ_RX_DONE_MASK           0x40

/* MODEM_CONFIG1 */

//bits 7-4
//bandwidths
#define BW_7_8KHZ   0 //7.8 kHz
#define BW_10_4KHZ  1 //10.4 kHz  
#define BW_15_6KHZ  2 //15.6 kHz
#define BW_20_8kHz  3 //20.8kHz
#define BW_31_25KHz 4 //31.25 kHz 
#define BW_41_7KHZ  5 //41.7 kHz
#define BW_62_5KHZ  6 //62.5 kHz
#define BW_125KHZ   7 //125 kHz
#define BW_250KHZ   8 //250 kHz
#define BW_500KHZ   9 //500 kHz

//bits 3-1
//error coding rate
#define CODING_RATE_4_5 1
#define CODING_RATE_4_6 2
#define CODING_RATE_4_7 3
#define CODING_RATE_4_8 4

//bit 0
//implicit header mode
#define HEADER_MODE_EXPLICIT 0
#define HEADER_MODE_IMPLICIT 1
/**********/

/* MODEM_CONFIG2 */
//bits 7-4
//spreading factor
#define SF_6  6  //  64 chips/symbol
#define SF_7  7  // 128 chips/symbol
#define SF_8  8  // 256 chips/symbol
#define SF_9  9  // 512 chips/symbol
#define SF_10 10 //1024 chips/symbol
#define SF_11 11 //2048 chips/symbol
#define SF_12 12 //4096 chips/symbol

//bit 3
//tx continuous mode
#define TX_CONTINUOUS_DISABLE 0 //normal mode, a single packet is sent
#define TX_CONTINUOUS_ENABLE  1 //continuous mode, send multiple packets across the FIFO 

//bit 2
//rx payload crc on
//Enable CRC generation and check on payload: 
//If CRC is needed, RxPayloadCrcOn should be set: 
//  - in Implicit header mode: on Tx and Rx side 
//  - in Explicit header mode: on the Tx side alone (recovered from the header in Rx side)
#define RX_PAYLOAD_CRC_DISABLE 0
#define RX_PAYLOAD_CRC_ENABLE  1

//bit 1-0
//symb timeout (9:8)
//RX Time-Out MSB 
#define SYMB_TIMEOUT 0
/**********/

/* MODEM_CONFIG3 */

//bits 7-4 unused

//bit 3
#define LOW_DATA_RATE_OPTIMIZE_DISABLED 0 //Disabled
#define LOW_DATA_RATE_OPTIMIZE_ENABLED  1 //Enabled; mandated for when the symbol length exceeds 16ms

//bit 2
#define ACT_AUTO_ON  0 //LNA gain set by register LnaGain
#define ACT_AUTO_OFF 1 //LNA gain set by the internal AGC loop

//bits 1-0 reserved
/**********/


#endif
