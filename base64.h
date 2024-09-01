/*
 * base64.h
 *
 *  Created on: 9 Aug 2024
 *      Author: cy1228
 */

#ifndef BSP_BASE_BASE64_H_
#define BSP_BASE_BASE64_H_
#include <stddef.h>

size_t Base64encode(char *encoded, const char *string, size_t len);
size_t Base64decode(char *bufplain, const char *bufcoded);


#endif /* BSP_BASE_BASE64_H_ */
