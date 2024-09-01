/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "../../BSP/RC522/RC522.h"
#include "../../BSP/R306/R306.h"
#include "../../BSP/ESP8266/ESP8266.h"
#include <string.h>
#define DEBOUNCE_DELAY 50
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, HAL_MAX_DELAY);  // Using USART2 for output
    return ch;
}

volatile uint8_t receive_state;
volatile uint8_t is_sleeping = 0;
uint8_t response[20];
volatile uint8_t wakeup_flag = 0;  // Flag to notify the main loop to handle wakeup
uint32_t last_activity_time = 0;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

    /* USER CODE BEGIN 1 */
    uint8_t getImageCommand[12] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
            0x00, 0x03, 0x01, 0x00, 0x05 };
    extern char newData[17];  // External variable from R307.c for new data
    extern volatile uint8_t data_ready;  // External variable declaration

    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART1_UART_Init();
    MX_SPI1_Init();
    MX_UART4_Init();
    MX_USART3_UART_Init();

    /* USER CODE BEGIN 2 */
    esp8266_init();
    /* Start UART receive interrupt */
    /* Connect to WiFi */
    connect_to_wifi(WIFI_SSID, WIFI_PASSWORD);
    get_and_display_ip_address();
    check_wifi_connection_status();
    RC522_Init();
    R306_Init();
    printf("Hello, STM32!\r\n");

    HAL_UARTEx_ReceiveToIdle_IT(&huart3, response, sizeof(response));
    uint8_t finger_pressed = 0;  // Flag to indicate if the finger is pressed
    receive_state = 1;
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
    HAL_Delay(500);
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
    last_activity_time = HAL_GetTick();  // Record the last activity time

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        if (wakeup_flag) {
            wakeup_flag = 0;  // Reset wakeup flag
            last_activity_time = HAL_GetTick();  // Update activity time to prevent immediate sleep
            WakeupHandler();  // Execute wakeup handler
        }

        // Check if the touch sensor detects a finger press
        if (HAL_GPIO_ReadPin(TOUCH_GPIO_GPIO_Port, TOUCH_GPIO_Pin) == GPIO_PIN_RESET && finger_pressed == 0) {
            HAL_Delay(DEBOUNCE_DELAY);  // Debounce delay

            if (HAL_GPIO_ReadPin(TOUCH_GPIO_GPIO_Port, TOUCH_GPIO_Pin) == GPIO_PIN_RESET) {
                printf("Finger detected, starting image capture.\n");

                R306_StartImageCapture();  // Start image capture
                finger_pressed = 1;  // Set flag to indicate finger is pressed
                last_activity_time = HAL_GetTick();
            }
        }

        // Reset flag when the finger is removed, allowing the next capture
        if (HAL_GPIO_ReadPin(TOUCH_GPIO_GPIO_Port, TOUCH_GPIO_Pin) == GPIO_PIN_SET && finger_pressed == 1) {
            HAL_Delay(DEBOUNCE_DELAY);  // Debounce delay

            if (HAL_GPIO_ReadPin(TOUCH_GPIO_GPIO_Port, TOUCH_GPIO_Pin) == GPIO_PIN_SET) {
                finger_pressed = 0;  // Reset flag for next press
                printf("Finger removed, ready for next capture.\n");
                last_activity_time = HAL_GetTick();  // Update activity time
            }
        }

        if (data_ready == 1) {
            // Connect to EC2 server and send data
            connect_to_ec2(ec2_host, ec2_port);
            HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
            HAL_Delay(500);
            HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
            sendFeatureDataToServer(newData);

            // Reset flags
            data_ready = 0;
            receive_state = 1;
            last_activity_time = HAL_GetTick();  // Update activity time
        }

        HAL_Delay(1000);

        uint8_t status;
        uint8_t str[MAX_LEN];  // Buffer for the result of the request command
        uint8_t serNum[5];
        status = RC522_Request(PICC_REQIDL, str);

        if (status == MI_OK) {
            last_activity_time = HAL_GetTick();
            HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
            HAL_Delay(500);
            HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);

            status = RC522_Anticoll(serNum);
            // Initialize UID string
            char UID[256];
            UID[0] = '\0';  // Initialize as empty string

            // Convert serNum bytes to characters and append to UID
            for (int i = 0; i <= 4; i++) {
                char temp[4];  // Temp array to store character representation of a byte
                sprintf(temp, "%02X", serNum[i]);  // Convert byte to hexadecimal representation
                strcat(UID, temp);  // Append to UID string
            }

            printf("UID=%s\r", UID);
            connect_to_ec2(ec2_host, ec2_port);
            send_UID_to_server_https(UID);
        }

        // Check for inactivity for 2 minutes, then enter sleep mode
        if ((HAL_GetTick() - last_activity_time) > 120000 && !is_sleeping) {
            printf("No activity for 2 minutes, entering sleep mode...\n");
            EnterSleepMode();
        }

        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    // Assume GPIO pin 1 triggers wakeup
    if (GPIO_Pin == GPIO_PIN_1) {
        if (is_sleeping) {
            is_sleeping = 0;  // Reset sleep flag
            wakeup_flag = 1;  // Set wakeup flag to notify the main loop
        } else {
            // Ignore interrupt in normal mode
            //printf("PIR sensor triggered, but ignored.\n");
        }
    }
}

void EnterSleepMode(void) {
    printf("Entering sleep mode...\n");

    is_sleeping = 1;  // Set flag to indicate sleep mode
    HAL_PWR_DisableSleepOnExit();  // Disable auto-exit from sleep mode
    SysTick->CTRL &= ~(SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk);  // Disable SysTick interrupt

    // MCU enters sleep mode, waiting for external interrupt to wake up
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

void WakeupHandler(void) {
    // Actions after exiting sleep mode
    printf("MCU woke up from sleep.\n");
    SysTick->CTRL |= (SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk);

    // Reinitialize peripherals
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART1_UART_Init();
    MX_SPI1_Init();
    MX_UART4_Init();
    MX_USART3_UART_Init();

    // Check WiFi connection status
    char response[128];
    send_AT_command("AT+CWJAP?\r\n", response, sizeof(response), 5000);

    if (strstr(response, "No AP") != NULL) {
        printf("WiFi not connected. Reconnecting...\n");
        esp8266_init();
        connect_to_wifi(WIFI_SSID, WIFI_PASSWORD);
        get_and_display_ip_address();
    } else {
        printf("WiFi already connected.\n");
    }

    // Reinitialize fingerprint and RC522
    R306_Init();
    HAL_UARTEx_ReceiveToIdle_IT(&huart3, response, sizeof(response));
    RC522_Init();
}

int is_wifi_connected(void) {
    char response[128];

    // Send command to check WiFi connection status
    send_AT_command("AT+CWJAP?\r\n", response, sizeof(response), 5000);

    // Check if response contains the connected SSID
    if (strstr(response, "No AP") != NULL) {
        return 0;  // Not connected to WiFi
    } else {
        return 1;  // Connected to WiFi
    }
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
