#ifndef _WALLET_H_
#define _WALLET_H_

#include <stdint.h>
#include <stdbool.h>

struct ByteStream_t;
uint8_t deserialize_cells_tree(struct ByteStream_t* src);
void get_address(const uint32_t account_number, uint8_t* address);

#endif
