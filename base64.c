/*
 * base.c
 *
 *  Created on: Aug 23, 2024
 *      Author: yupili
 */
// References:
	// - Example from “How do I base64 encode (decode) in C?,” Stack Overflow. https://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c

#include "base64.h"

const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void encodeBase64(const uint8_t *data, size_t dataSize, char *encodedData) {
    int i = 0, j = 0;
    uint8_t arr_3[3], arr_4[4];

    while (dataSize--) {
        arr_3[i++] = *(data++);
        if (i == 3) {
            arr_4[0] = (arr_3[0] & 0xfc) >> 2;
            arr_4[1] = ((arr_3[0] & 0x03) << 4) + ((arr_3[1] & 0xf0) >> 4);
            arr_4[2] = ((arr_3[1] & 0x0f) << 2) + ((arr_3[2] & 0xc0) >> 6);
            arr_4[3] = arr_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                encodedData[j++] = base64_table[arr_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (int k = i; k < 3; k++)
            arr_3[k] = '\0';

        arr_4[0] = (arr_3[0] & 0xfc) >> 2;
        arr_4[1] = ((arr_3[0] & 0x03) << 4) + ((arr_3[1] & 0xf0) >> 4);
        arr_4[2] = ((arr_3[1] & 0x0f) << 2) + ((arr_3[2] & 0xc0) >> 6);
        arr_4[3] = arr_3[2] & 0x3f;

        for (int k = 0; (k < i + 1); k++)
            encodedData[j++] = base64_table[arr_4[k]];

        while ((i++ < 3))
            encodedData[j++] = '=';
    }
    encodedData[j] = '\0';
}


