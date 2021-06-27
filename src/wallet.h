#ifndef _WALLET_H_
#define _WALLET_H_

#include <stdint.h>
#include <stdbool.h>

struct ByteStream_t;
uint8_t deserialize_cells_tree(struct ByteStream_t* src);
void get_address(const uint32_t account_number, uint8_t* address);
void address_to_string(int8_t wc, uint8_t flags, uint8_t* address, uint8_t in_len, uint8_t out_len, char* str);

#define USER_FRIENDLY 0x1
#define URL_SAFE 0x2
#define BOUNCEABLE 0x4
#define TEST_ONLY 0x8

#endif
