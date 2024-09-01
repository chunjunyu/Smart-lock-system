/*
 * R306.c
 *
 *  Created on: Aug 23, 2024
 *      Author: yupili
 */

#include "R306.h"
#include "usart.h"
#include "gpio.h"
#include "../../BSP/Base/base64.h"
#include "../../BSP/ESP8266/ESP8266.h"
#include <string.h>
#include <stdio.h>

#define NUM_SAMPLES 3
#define FEATURE_LENGTH 512

static uint8_t featureData[FEATURE_LENGTH];  // Buffer to store fingerprint feature data
static uint8_t response[20];  // Buffer to store responses from the R306 module
CommandType currentState = STATE_IDLE;  // Current state of the fingerprint sensor
char base64EncodedData[17];  // Buffer for storing Base64 encoded data
char newData[17];  // Buffer to store copied Base64 encoded data
volatile uint8_t data_ready = 0;  // Flag indicating if the data is ready
extern volatile uint8_t receive_state;  // External state to indicate if the receive process is active

void R306_Init(void) {
    currentState = STATE_IDLE;  // Initialize the fingerprint sensor state to idle
}

void R306_StartImageCapture(void) {
    if (currentState == STATE_IDLE && receive_state == 1) {
        // Send command to capture fingerprint image
        uint8_t getImageCommand[] = {
            0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
            0x00, 0x03, 0x01, 0x00, 0x05
        };
        HAL_UART_Transmit(&huart3, getImageCommand, sizeof(getImageCommand), HAL_MAX_DELAY);
        currentState = STATE_WAIT_IMAGE;  // Update the state to waiting for image capture
        HAL_UARTEx_ReceiveToIdle_IT(&huart3, response, 20);  // Start reception of the response
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == USART3) {
        response[Size] = '\0';  // Null-terminate the received data

        if (response[9] == 0x00) {  // Check if the command was successful
            switch (currentState) {
                case STATE_WAIT_IMAGE:
                    printf("Fingerprint image captured successfully.\n");
                    currentState = STATE_EXTRACT_FEATURES;
                    uint8_t image2TzCommand[] = {
                        0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
                        0x00, 0x04, 0x02, 0x01, 0x00, 0x08
                    };
                    HAL_UART_Transmit(&huart3, image2TzCommand, sizeof(image2TzCommand), HAL_MAX_DELAY);
                    break;

                case STATE_EXTRACT_FEATURES:
                    printf("Fingerprint feature extraction successful.\n");
                    currentState = STATE_READ_FEATURES;
                    uint8_t readCharBuffer1Command[] = {
                        0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
                        0x00, 0x04, 0x08, 0x01, 0x00, 0x0E
                    };
                    HAL_UART_Transmit(&huart3, readCharBuffer1Command, sizeof(readCharBuffer1Command), HAL_MAX_DELAY);
                    break;

                case STATE_READ_FEATURES:
                    printf("Fingerprint feature read successfully.\n");
                    currentState = STATE_IDLE;

                    // Receive fingerprint feature data
                    HAL_UART_Receive(&huart3, featureData, sizeof(featureData), HAL_MAX_DELAY);

                    // Print a portion of the feature data for debugging
                    printf("featureData (part):\n");
                    for (int i = 111; i <= 122; i++) {
                        printf("%02X ", featureData[i]);
                    }
                    printf("\n");

                    // Base64 encode a part of the feature data
                    encodeBase64(&featureData[111], 10, base64EncodedData);
                    printf("Base64 Encoded Data: %s\n", base64EncodedData);

                    // Copy encoded data to newData buffer
                    strcpy(newData, base64EncodedData);

                    // Set flags to notify that the data is ready
                    data_ready = 1;
                    receive_state = 0;
                    break;

                default:
                    break;
            }
        } else if (response[9] == 0x82) {  // Handle specific error response
            printf("Command failed with response code: 0x82 (Invalid register number or page addressing error)\n");
            uint8_t resetCommand[] = {
                0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
                0x00, 0x03, 0x0D, 0x00, 0x11
            };
            HAL_UART_Transmit(&huart3, resetCommand, sizeof(resetCommand), HAL_MAX_DELAY);
            for (int i = 0; i <= 10; i++) {
                printf("WAITING....");
            }
            R306_StartImageCapture();  // Restart the image capture process
        } else {  // Handle other errors
            printf("Command failed with response code: 0x%02X\n", response[9]);
            currentState = STATE_IDLE;
        }

        // Re-enable idle receive interrupt
        HAL_UARTEx_ReceiveToIdle_IT(&huart3, response, 20);
    }
}
