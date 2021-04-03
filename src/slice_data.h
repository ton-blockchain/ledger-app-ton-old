#ifndef _SLICE_DATA_H_
#define _SLICE_DATA_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct SliceData_t {
    uint8_t* data;
    uint16_t data_window_start;
    uint16_t data_window_end;
    uint16_t data_size_bytes;
} SliceData_t;

void SliceData_init(struct SliceData_t* self, uint8_t* data, uint16_t data_size_bytes);
void SliceData_fill(struct SliceData_t* self, uint8_t value, uint16_t data_size_bytes);
void SliceData_truncate(struct SliceData_t* self, uint16_t size);
uint16_t SliceData_remaining_bits(const struct SliceData_t* self);
void SliceData_move_by(struct SliceData_t* self, uint16_t offset);
uint8_t SliceData_get_bits(const struct SliceData_t* self, uint16_t offset, uint8_t bits);
uint8_t SliceData_get_next_bit(struct SliceData_t* self);
uint8_t SliceData_get_next_byte(struct SliceData_t* self);
uint64_t SliceData_get_next_int(struct SliceData_t* self, uint8_t bits);
uint64_t SliceData_get_next_size(struct SliceData_t* self, uint16_t max_value);
bool SliceData_is_empty(const struct SliceData_t* self);
bool SliceData_equal(const struct SliceData_t* self, const struct SliceData_t* other);
uint16_t SliceData_get_cursor(const struct SliceData_t* self);

#endif
