/*
 * ESP8266.h
 *
 *  Created on: 6 Aug 2024
 *      Author: gvr509
 */

#ifndef BSP_ESP8266_ESP8266_H_
#define BSP_ESP8266_ESP8266_H_

#include <stdint.h>

// Define constants for ESP8266
//#define WIFI_SSID "iPhone"       // Replace with your WiFi name
//#define WIFI_PASSWORD "1273656808" // Replace with your WiFi password
#define WIFI_SSID "ycjdeshouji"       // Replace with your WiFi name
#define WIFI_PASSWORD "18258552039"
//#define WIFI_SSID "HUAWEIMate60pro"       // Replace with your WiFi name
//#define WIFI_PASSWORD "18258552039" // Replace with your WiFi password
#define ec2_host   "ec2-13-61-25-245.eu-north-1.compute.amazonaws.com"
#define ec2_port   80
#define HTTPS_PORT 443                  // HTTPS port

// Public function declarations
void send_AT_command(const char* cmd, char* response, size_t response_size, int timeout);
void get_and_display_ip_address();
void connect_to_wifi(char* ssid, char* password);
void start_transparent_mode(const char* ip, int port);
void send_data_via_transparent_mode(const char* data);
void check_wifi_connection_status(void);
void connect_to_AWS(void);
void send_UID_to_server_https(const char* uid);
void sendFeatureDataToServer(const char* encodedFeatureData);

void get_firmware_version(void);
void get_connection_status(void);
void esp8266_init(void);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

#endif /* BSP_ESP8266_ESP8266_H_ */
