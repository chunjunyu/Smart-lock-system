/*
 * RC522.c
 *
 *  Created on: Jul 25, 2024
 *      Author: cy1228
 */

#include "rc522.h"
#include "spi.h"
#include "gpio.h"

void RC522_Init(void) {
	// Reset RC522
	HAL_GPIO_WritePin(GPIOA, RST_Pin, GPIO_PIN_RESET); // Pull the reset pin low
	HAL_Delay(50);  // Wait for 50ms
	HAL_GPIO_WritePin(GPIOA, RST_Pin, GPIO_PIN_SET);  // Pull the reset pin high
	HAL_Delay(50);  // Wait for 50ms

	// Configure RC522 registers
	RC522_WriteRegister(TModeReg, 0x8D); // Set TModeReg to recommended settings
	RC522_WriteRegister(TPrescalerReg, 0x3E); // Set TPrescalerReg to recommended settings
	RC522_WriteRegister(TReloadRegL, 0x30);  // Set Reload timer low byte
	RC522_WriteRegister(TReloadRegH, 0x00);  // Set Reload timer high byte
	RC522_WriteRegister(TxASKReg, 0x40);  // Force 100% ASK modulation
	RC522_WriteRegister(ModeReg, 0x3D); // Configure ModeReg for 14443A standard compliance
	// References:
	// - Example from [“RFID RC522 i STM32 BluePill - FX-Team,” FX-Team, Dec. 28, 2020. https://fx-team.fulara.com/rf522-i-stm32-bluepill/ (accessed Aug. 31, 2024).]

	// Enable antenna
	RC522_AntennaOn();
}

void RC522_AntennaOn(void) {
	uint8_t temp;
	temp = RC522_ReadRegister(TxControlReg);
	if ((temp & 0x03) != 0x03) {
		RC522_SetBitMask(TxControlReg, 0x03);
		temp = RC522_ReadRegister(TxControlReg);
		temp &= ~0x03;
		temp |= 0x03;
		RC522_WriteRegister(TxControlReg, temp);
		HAL_Delay(100);
	}
}

uint8_t RC522_CheckAntennaOn(void) {
	uint8_t temp;
	temp = RC522_ReadRegister(TxControlReg);
	return (temp & 0x03) == 0x03;
}

static uint8_t ret;
uint8_t SPI1_RW_Byte(uint8_t byte) {
	// Hardware SPI: Transmit and receive one byte
	HAL_SPI_TransmitReceive(&hspi1, &byte, &ret, 1, 10);
	return ret;
}

void RC522_WriteRegister(uint8_t reg, uint8_t value) {
	uint8_t addr = (reg << 1) & 0x7E; // Prepare the address byte for writing (MSB = 0)
	HAL_GPIO_WritePin(GPIOA, RC522_CS_Pin, GPIO_PIN_RESET); // Select RC522
	SPI1_RW_Byte(addr);  // Send the address byte
	SPI1_RW_Byte(value); // Write the value
	HAL_GPIO_WritePin(GPIOA, RC522_CS_Pin, GPIO_PIN_SET);   // Deselect RC522
}

uint8_t RC522_ReadRegister(uint8_t addr) {
	uint8_t regAddr = ((addr << 1) & 0x7E) | 0x80; // Prepare the address byte for reading (MSB = 1)
	uint8_t value;

	HAL_GPIO_WritePin(GPIOA, RC522_CS_Pin, GPIO_PIN_RESET); // Select RC522
	SPI1_RW_Byte(regAddr);  // Send the address byte
	value = SPI1_RW_Byte(0x00);  // Read the value
	HAL_GPIO_WritePin(GPIOA, RC522_CS_Pin, GPIO_PIN_SET);   // Deselect RC522

	return value;
}

uint8_t RC522_Request(uint8_t reqMode, uint8_t *TagType) {
	uint8_t status;
	uint16_t backBits;  // Number of received data bits

	RC522_WriteRegister(BitFramingReg, 0x07);  // Set the bit framing register

	TagType[0] = reqMode;  // Set the request mode
	RC522_ClearBitMask(Status2Reg, 0x08);  // Clear the receiver start bit
	RC522_WriteRegister(FIFOLevelReg, 0x80);  // Clear the FIFO buffer
	RC522_WriteRegister(CommandReg, PCD_IDLE);  // Clear the FIFO
	RC522_ClearBitMask(CommIrqReg, 0x80);  // Clear all interrupt flags
	RC522_WriteRegister(CommIEnReg, 0x77);  // Enable all interrupts
	status = RC522_ToCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);

	if ((status != MI_OK) || (backBits != 0x10)) // Check if the received data is 16 bits (2 bytes)
			{
		status = MI_ERR;
	}

	return status;
}

// Function: RC522_ToCard
// Purpose: Handles communication with the RC522 RFID module, including data transmission and reception.
// Reference: This implementation is based on the example provided in the RC522 datasheet, section 4.3.1
//            and inspired by the tutorial from [“RFID RC522 i STM32 BluePill - FX-Team,” FX-Team, Dec. 28, 2020. https://fx-team.fulara.com/rf522-i-stm32-bluepill/ (accessed Aug. 31, 2024).]
uint8_t RC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen,
		uint8_t *backData, uint16_t *backLen) {
	uint8_t status = MI_ERR;  // Default status to indicate an error
	uint8_t irqEnable = 0x00; // Interrupt enable configuration
	uint8_t waitForInterrupt = 0x00; // Interrupt waiting flag
	uint8_t lastBits;
	uint8_t receivedCount;
	uint16_t i;
	uint16_t timeout = 2000;  // Set a timeout value

	// Determine the appropriate interrupt flags based on the command
	switch (command) {
	case PCD_AUTHENT:  // Authentication command
		irqEnable = 0x12;  // Enable interrupts related to authentication
		waitForInterrupt = 0x10; // Wait for authentication to complete
		break;

	case PCD_TRANSCEIVE:  // Transmit and receive command
		irqEnable = 0x77;  // Enable transmit and receive interrupts
		waitForInterrupt = 0x30; // Wait for transmission/reception to complete
		break;

	default:
		break;
	}

	// Initialize the communication by configuring the interrupts and clearing buffers
	RC522_WriteRegister(CommIEnReg, irqEnable | 0x80); // Enable the configured interrupts
	RC522_ClearBitMask(CommIrqReg, 0x80);  // Clear any pending interrupt flags
	RC522_SetBitMask(FIFOLevelReg, 0x80);  // Clear the FIFO buffer

	// Load data into the FIFO buffer
	for (i = 0; i < sendLen; i++) {
		RC522_WriteRegister(FIFODataReg, sendData[i]);
	}

	// Execute the command by writing to the CommandReg register
	RC522_WriteRegister(CommandReg, command);

	if (command == PCD_TRANSCEIVE) {
		// Start transmission by setting the start-send bit in BitFramingReg
		RC522_SetBitMask(BitFramingReg, 0x80);
	}

	// Wait for the process to complete by monitoring the interrupt flags
	while ((timeout--) && !(RC522_ReadRegister(CommIrqReg) & waitForInterrupt)
			&& !(RC522_ReadRegister(CommIrqReg) & 0x01)) {
		// Waiting for the command to complete or timeout
	}

	// Clear the transmission bit if the command was to transmit data
	RC522_ClearBitMask(BitFramingReg, 0x80);

	// Evaluate whether the operation was successful
	if (timeout && !(RC522_ReadRegister(ErrorReg) & 0x1B)) {
		status = MI_OK;  // Update status to indicate success

		if (command == PCD_TRANSCEIVE) {
			// Retrieve the number of bytes in the FIFO and process the received data
			receivedCount = RC522_ReadRegister(FIFOLevelReg);
			lastBits = RC522_ReadRegister(ControlReg) & 0x07;

			if (lastBits) {
				*backLen = (receivedCount - 1) * 8 + lastBits;
			} else {
				*backLen = receivedCount * 8;
			}

			if (receivedCount == 0) {
				receivedCount = 1;
			}
			if (receivedCount > MAX_LEN) {
				receivedCount = MAX_LEN;
			}

			// Copy the received data from the FIFO buffer
			for (i = 0; i < receivedCount; i++) {
				backData[i] = RC522_ReadRegister(FIFODataReg);
			}
		}
	} else {
		status = MI_ERR;  // If an error occurred, retain the error status
	}

	return status;  // Return the final status of the operation
}

void RC522_ClearBitMask(uint8_t reg, uint8_t mask) {
	uint8_t tmp = RC522_ReadRegister(reg);
	RC522_WriteRegister(reg, tmp & (~mask)); // Clear specific bits
}

void RC522_SetBitMask(uint8_t reg, uint8_t mask) {
	uint8_t tmp = RC522_ReadRegister(reg);
	RC522_WriteRegister(reg, tmp | mask); // Set specific bits
}

uint8_t RC522_Anticoll(uint8_t *serNum) {
	uint8_t status;
	uint8_t i;
	uint8_t serNumCheck = 0;
	uint16_t unLen;

	RC522_WriteRegister(BitFramingReg, 0x00); // Clear bit framing, set to standard mode

	serNum[0] = PICC_ANTICOLL;  // Anti-collision command
	serNum[1] = 0x20;           // Anti-collision command parameter
	RC522_WriteRegister(FIFOLevelReg, 0x80);  // Clear FIFO buffer
	RC522_WriteRegister(CommandReg, PCD_IDLE);  // Clear FIFO
	RC522_ClearBitMask(CommIrqReg, 0x80);  // Clear all interrupt flags
	RC522_WriteRegister(CommIEnReg, 0x77);  // Enable all interrupts
	status = RC522_ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

	if (status == MI_OK) {
		// Calculate the checksum
		for (i = 0; i < 4; i++) {
			serNumCheck ^= serNum[i];
		}
		// Verify the checksum
		if (serNumCheck != serNum[i]) {
			status = MI_ERR;
		}
	}
	return status;
}
