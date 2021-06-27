#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <stdint.h>

struct ByteStream_t;
void deserialize_message(struct ByteStream_t* src, uint8_t display_flags);

#endif
