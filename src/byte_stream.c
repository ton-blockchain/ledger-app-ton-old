#include "byte_stream.h"
#include "errors.h"
#include "utils.h"

void ByteStream_init(struct ByteStream_t* self, uint8_t* data, uint16_t data_size) {
    VALIDATE(self && data && data_size, ERR_INVALID_DATA);
    self->data_size = data_size;
    self->offset = 0;
    self->data = data;
}

void ByteStream_move_by(struct ByteStream_t* self, uint16_t data_size) {
    VALIDATE(data_size <= self->data_size - self->offset, ERR_END_OF_STREAM);
    self->offset += data_size;
}

uint8_t* ByteStream_read_data(struct ByteStream_t* self, uint32_t data_size) {
    uint8_t* data = self->data + self->offset;
    ByteStream_move_by(self, data_size);
    return data;
}

uint8_t ByteStream_read_byte(struct ByteStream_t* self) {
    uint8_t byte = self->data[self->offset];
    ByteStream_move_by(self, 1);
    return byte;
}

uint32_t ByteStream_read_u32(struct ByteStream_t* self) {
    uint32_t u32 = readUint32BE(self->data + self->offset);
    ByteStream_move_by(self, sizeof(uint32_t));
    return u32;
}

uint8_t* ByteStream_get_cursor(struct ByteStream_t* self) {
    return self->data + self->offset;
}
