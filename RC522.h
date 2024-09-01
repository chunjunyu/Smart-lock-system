/*
 * RC522.h
 *
 *  Created on: Jul 25, 2024
 *      Author: cy1228
 */

#ifndef BSP_RC522_RC522_H_
#define BSP_RC522_RC522_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

// RC522 Register Definitions
#define CommandReg             0x01
#define CommIEnReg             0x02
#define DivIEnReg              0x03
#define CommIrqReg             0x04
#define DivIrqReg              0x05
#define ErrorReg               0x06
#define Status1Reg             0x07
#define Status2Reg             0x08
#define FIFODataReg            0x09
#define FIFOLevelReg           0x0A
#define WaterLevelReg          0x0B
#define ControlReg             0x0C
#define BitFramingReg          0x0D
#define CollReg                0x0E
#define ModeReg                0x11
#define TxControlReg           0x14
#define TxASKReg               0x15
#define CRCResultRegM          0x21
#define CRCResultRegL          0x22
#define TModeReg               0x2A
#define TPrescalerReg          0x2B
#define TReloadRegH            0x2C
#define TReloadRegL            0x2D
#define VersionReg             0x37

// RC522 Command Definitions
#define PCD_IDLE               0x00    // No action, cancel the current command
#define PCD_AUTHENT            0x0E    // Authenticate key
#define PCD_RECEIVE            0x08    // Receive data
#define PCD_TRANSMIT           0x04    // Transmit data
#define PCD_TRANSCEIVE         0x0C    // Transmit and receive data
#define PCD_RESETPHASE         0x0F    // Reset
#define PCD_CALCCRC            0x03    // CRC calculation

// PICC Command Definitions
#define PICC_REQIDL            0x26    // Request idle state cards in the antenna area
#define PICC_REQALL            0x52    // Request all cards in the antenna area
#define PICC_ANTICOLL          0x93    // Anti-collision
#define PICC_SElECTTAG         0x93    // Select card
#define PICC_AUTHENT1A         0x60    // Authenticate A key
#define PICC_AUTHENT1B         0x61    // Authenticate B key
#define PICC_READ              0x30    // Read block
#define PICC_WRITE             0xA0    // Write block
#define PICC_DECREMENT         0xC0    // Decrement
#define PICC_INCREMENT         0xC1    // Increment
#define PICC_RESTORE           0xC2    // Restore block data to buffer
#define PICC_TRANSFER          0xB0    // Save buffer data to the block
#define PICC_HALT              0x50    // Halt

// Status Codes
#define MI_OK                  0       // Success
#define MI_NOTAGERR            1       // No tag error
#define MI_ERR                 2       // Error
#define MAX_LEN                16      // Max length of a data buffer

// Function Declarations
void RC522_Init(void);
void RC522_AntennaOn(void);
void RC522_AntennaOff(void);
void RC522_Reset(void);

void RC522_WriteRegister(uint8_t reg, uint8_t value);
uint8_t RC522_ReadRegister(uint8_t reg);

void RC522_SetBitMask(uint8_t reg, uint8_t mask);
void RC522_ClearBitMask(uint8_t reg, uint8_t mask);

uint8_t RC522_Request(uint8_t reqMode, uint8_t *TagType);
uint8_t RC522_Anticoll(uint8_t *serNum);
uint8_t RC522_SelectTag(uint8_t *serNum);
uint8_t RC522_Auth(uint8_t authMode, uint8_t BlockAddr, uint8_t *Sectorkey, uint8_t *serNum);
uint8_t RC522_Read(uint8_t blockAddr, uint8_t *recvData);
uint8_t RC522_Write(uint8_t blockAddr, uint8_t *writeData);
uint8_t RC522_CheckAntennaOn(void);
uint8_t SPI1_RW_Byte(uint8_t byte);
void RC522_Halt(void);

void RC522_CalculateCRC(uint8_t *pIndata, uint8_t len, uint8_t *pOutData);
uint8_t RC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint16_t *backLen);

#ifdef __cplusplus
}
#endif

#endif /* BSP_RC522_RC522_H_ */

