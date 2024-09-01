/*
 * R306.h
 *
 *  Created on: Aug 23, 2024
 *      Author: yupili
 */

#ifndef BSP_R306_R306_H_
#define BSP_R306_R306_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

// Define instruction codes for the R306 fingerprint sensor
#define GR_GetImage          0x01  // Read an image from the sensor and store it in the image buffer
#define GR_GenChar           0x02  // Generate a fingerprint feature file from the image and store it in CharBuffer1 or CharBuffer2
#define GR_Match             0x03  // Match the feature files in CharBuffer1 and CharBuffer2
#define GR_Search            0x04  // Search the entire or part of the fingerprint library using the feature file in CharBuffer1 or CharBuffer2
#define GR_RegModel          0x05  // Merge feature files from CharBuffer1 and CharBuffer2 to generate a template and store it in both buffers
#define GR_StoreChar         0x06  // Store the file from the feature buffer to the flash fingerprint library
#define GR_LoadChar          0x07  // Load a fingerprint template from flash to the feature buffer
#define GR_UpChar            0x08  // Upload a fingerprint template from the module to the host
#define GR_DownChar          0x09  // Download a fingerprint template from the host to the module
#define GR_UpImage           0x0A  // Upload an image from the module to the host
#define GR_DownImage         0x0B  // Download an image from the host to the module
#define GR_DeletChar         0x0C  // Delete a fingerprint template from the library
#define GR_Empty             0x0D  // Clear the fingerprint library
#define GR_WriteReg          0x0E  // Write to system register
#define GR_ReadSysPara       0x0F  // Read system parameters
#define GR_Enroll            0x10  // Enroll a new fingerprint template
#define GR_Identify          0x11  // Identify a fingerprint by comparing with all templates in the library
#define GR_SetPwd            0x12  // Set the module's password
#define GR_VfyPwd            0x13  // Verify the module's password
#define GR_GetRandomCode     0x14  // Get a random code from the module
#define GR_SetAddr           0x15  // Set the module's address
#define GR_Port_Control      0x17  // Control the IO ports
#define GR_WriteNotepad      0x18  // Write data to the notepad
#define GR_ReadNotepad       0x19  // Read data from the notepad
#define GR_HighSpeedSearch   0x1B  // Perform a high-speed search of the fingerprint library
#define GR_GenBinImage       0x1C  // Generate a binary image from the image buffer
#define GR_ValidTempleteNum  0x1D  // Get the number of valid templates in the fingerprint library

// Function prototypes
void R306_SendCommand(uint8_t* command, uint16_t length);
void R306_ReceiveResponse(uint8_t* response, uint16_t length);
uint16_t R306_CalculateChecksum(uint8_t* data, uint16_t length);
HAL_StatusTypeDef R306_EnrollFingerprint(uint16_t pageId);
HAL_StatusTypeDef R306_MatchFingerprint(uint16_t startPageId, uint16_t endPageId);
uint8_t* R306_GetReceivedData(void);
void R306_BlockingReceive(void);
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);
void R306_Init(void);
void R306_StartImageCapture(void);

// Enumeration for command types and states
typedef enum {
    COMMAND_NONE,            // No command
    STATE_WAIT_IMAGE,        // Waiting for image capture
    STATE_EXTRACT_FEATURES,  // Extracting features from the captured image
    STATE_READ_FEATURES,     // Reading features from the module
    STATE_IDLE               // Idle state
} CommandType;

extern CommandType currentState;  // External variable for tracking the current state

#ifdef __cplusplus
}
#endif

#endif /* BSP_R306_R306_H_ */
