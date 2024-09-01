#include "usart.h"
#include "ESP8266.h"
#include <string.h>
#include <stdio.h>

static char rx_buffer[256];         // Receive buffer
static volatile uint8_t rx_flag = 0;

// Buffer to store the complete response
static char response_buffer[4096];
static volatile size_t response_index = 0;
static volatile uint8_t response_complete = 0;
static size_t expected_length = 0;  // Data length obtained from the +IPD part
static size_t received_length = 0;  // Actually received data length


void send_AT_command(const char* cmd, char* response, size_t response_size, int timeout)
{
    // Clear the receive buffer and status
    memset(response, 0, response_size);
    memset(response_buffer, 0, sizeof(response_buffer));
    response_index = 0;
    rx_flag = 0;
    response_complete = 0;
    expected_length = 0;
    received_length = 0;

    // Send the AT command
    HAL_UART_Transmit(&huart4, (uint8_t*)cmd, strlen(cmd), 1000);

    // Start interrupt reception
    HAL_UART_Receive_IT(&huart4, (uint8_t*)rx_buffer, 1);

    // Wait for response or timeout
    uint32_t startTick = HAL_GetTick();
    while (!response_complete && (HAL_GetTick() - startTick < timeout))
    {
        // Wait for the receive flag to be set
        if (rx_flag)
        {
            rx_flag = 0;

            // Check if the complete response has been received
            if (response_complete)
            {
                break;
            }
            // Restart interrupt reception
            HAL_UART_Receive_IT(&huart4, (uint8_t*)rx_buffer, 1);
        }
    }

    // Copy the complete response to the user-provided buffer
    strncpy(response, response_buffer, response_size - 1);
    response[response_size - 1] = '\0';  // Ensure the string ends with '\0'
}

// Status enumeration for differentiating response types
typedef enum {
    RESPONSE_UNKNOWN,
    RESPONSE_AT,
    RESPONSE_HTTP
} ResponseType;

static ResponseType current_response_type = RESPONSE_UNKNOWN;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART4)
    {
        // Store the received character in the response buffer
        if (response_index < sizeof(response_buffer) - 1)
        {
            response_buffer[response_index++] = rx_buffer[0];
        }

        // Check if the response type has been determined
        if (current_response_type == RESPONSE_UNKNOWN)
        {
            // Attempt to determine the response type
            if (strstr(response_buffer, "+IPD,") != NULL)
            {
                current_response_type = RESPONSE_HTTP;
            }
            else if (response_index >= 4 &&
                    (strncmp(&response_buffer[response_index - 4], "OK\r\n", 4) == 0 ||
                     strncmp(&response_buffer[response_index - 7], "ERROR\r\n", 7) == 0))
            {
                current_response_type = RESPONSE_AT;
            }
        }

        // Process data based on the determined response type
        if (current_response_type == RESPONSE_HTTP)
        {
            // Handle +IPD prefix (i.e., HTTP response)
            if (response_index >= 5 && expected_length == 0)
            {
                char *ipd_start = strstr(response_buffer, "+IPD,");
                if (ipd_start != NULL)
                {
                    // Parse the length value after +IPD
                    expected_length = atoi(ipd_start + 5);
                    received_length = response_index - (ipd_start - response_buffer) - strlen("+IPD,XXX:");
                }
            }
            else if (expected_length > 0)
            {
                received_length++;
            }

            // Check for chunked encoding
            if (strstr(response_buffer, "Transfer-Encoding: chunked") != NULL)
            {
                // Check if the final chunked transfer end flag "0\r\n\r\n" has been received
                if (strstr(response_buffer, "\r\n0\r\n\r\n") != NULL)
                {
                    response_complete = 1;
                }
            }

            // For HTTP response, check if the complete response has been received
            if (expected_length > 0 && received_length >= expected_length)
            {
                response_complete = 1;
            }
        }
        else if (current_response_type == RESPONSE_AT)
        {
            // Handle AT command responses, check common terminators "OK\r\n" or "ERROR\r\n"
            if (response_index >= 4 &&
                (strncmp(&response_buffer[response_index - 4], "OK\r\n", 4) == 0 ||
                 strncmp(&response_buffer[response_index - 7], "ERROR\r\n", 7) == 0))
            {
                response_complete = 1;
            }
        }

        // Set the flag and restart interrupt reception
        rx_flag = 1;

    }
}

void esp8266_init()
{
    // Initialize interrupt reception
    HAL_UART_Receive_IT(&huart4, (uint8_t*)rx_buffer, 1);

    char response[256];

    // Send command and check the response for success
    send_AT_command("ATE0\r\n", response, sizeof(response), 1000);

    // Check if the response contains a successful response
    if (strstr(response, "OK") == NULL) {
        printf("Failed to send ATE0 command to ESP8266.\n");
        return;  // Exit the function
    }

    // Delay for 100ms
    HAL_Delay(100);
}

// Function to connect to WiFi
void connect_to_wifi(char* ssid, char* password) {
    char cmd[100];
    char response[256];

    send_AT_command("AT+RST\r\n", response, sizeof(response), 1000);
    HAL_Delay(1000);

    // Test AT command
    send_AT_command("AT\r\n", response, sizeof(response), 1000);
    printf("Response to AT: %s\n", response);
    HAL_Delay(1000);

    // Set WiFi mode
    send_AT_command("AT+CWMODE=1\r\n", response, sizeof(response), 1000);
    HAL_Delay(1000);

    // Connect to WiFi
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    send_AT_command(cmd, response, sizeof(response), 5000);
    printf("Response to WIFI: %s\n", response);  // Print the response
    HAL_Delay(5000);
}

// Function to get and display the IP address
void get_and_display_ip_address() {
    char response[100];

    // Send the AT command to get the IP address
    send_AT_command("AT+CIFSR\r\n", response, sizeof(response), 1000);
    HAL_Delay(1000);

    // Print the response received from ESP8266
    printf("ESP8266 Response: %s\n", response);

    // Extract and display the IP address
    char* ip_start = strstr(response, "STAIP,\"");
    if (ip_start != NULL) {
        ip_start += 7;  // Skip "STAIP,\""
        char* ip_end = strchr(ip_start, '\"');
        if (ip_end != NULL) {
            *ip_end = '\0';  // Replace the ending quote with a string terminator
            printf("IP Address: %s\n", ip_start);
        } else {
            printf("Failed to parse IP address.\n");
        }
    } else {
        printf("No IP address found in response.\n");
    }
}

void connect_to_ec2(char* ec2_Host, char* ec2_Port)
{
    // Establish a TCP connection to EC2
    char cmd[256];
    char response[1024];
    sprintf(cmd, "AT+CIPSTART=\"SSL\",\"%s\",%d\r\n", ec2_Host, ec2_Port);
    send_AT_command(cmd, response, sizeof(response), 5000);
    printf("Response to TCP:\n%s\n", response);
}

void send_UID_to_server_https(const char* uid) {
    char response[256];
    char server_response[4096];  // Buffer to receive the server response

    // Construct JSON data, inserting the passed UID into the pass field
    char json_data[256];
    sprintf(json_data, "{\"pass\":\"%s\",\"openType\":\"1\"}", uid);

    // Construct HTTP POST request
    char http_request[512];
    sprintf(http_request,
            "POST /prod-api/door/lock/openByPass HTTP/1.1\r\n"
            "Host: 13.61.25.245\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n\r\n"
            "%s\r\n",
            strlen(json_data), json_data);

    // Tell the ESP8266 the length of the data to send
    char cip_send_cmd[64];
    sprintf(cip_send_cmd, "AT+CIPSEND=%d\r\n", strlen(http_request));
    send_AT_command(cip_send_cmd, response, sizeof(response), 5000);
    HAL_Delay(1000); // Wait for ESP8266 to be ready to send data

    // Send the
