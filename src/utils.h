#ifndef _UTILS_H_
#define _UTILS_H_

#include "os.h"
#include "cx.h"
#include "globals.h"

#include <stdint.h>
#include <stdbool.h>

unsigned int ui_prepro(const bagl_element_t *element);
void get_public_key(uint32_t accountNumber, uint8_t* publicKeyArray);
uint32_t readUint32BE(uint8_t *buffer);
void get_private_key(uint32_t accountNumber, cx_ecfp_private_key_t *privateKey);
void send_response(uint8_t tx, bool approve);
uint8_t leading_zeros(uint16_t value);
uint8_t convert_hex_amount_to_displayable(uint8_t* amount, uint8_t amount_length, char* out);
uint16_t crc16(char *ptr, int count);
size_t base64_encode(const uint8_t *data, int data_length, char *out);

#define VALIDATE(cond, error) \
    do {\
        if (!(cond)) { \
            PRINTF("Validation Error in %s: %d\n", __FILE__, __LINE__); \
            THROW(error); \
        } \
    } while(0)

#define PRINT_LINE() PRINTF("Print line %s: %d\n", __FILE__, __LINE__)

#endif
