#include "message.h"
#include "utils.h"
#include "error.h"
#include "slice_data.h"
#include "byte_stream.h"

#define BOC_GENERIC_TAG 0xb5ee9c72
#define MAX_ROOTS_COUNT 1
#define ADDRESS_CELL_INDEX 1

uint16_t deserialize_cell(struct Cell_t* cell, const uint8_t cell_index, const uint8_t cells_count) {
    uint8_t d1 = Cell_get_d1(cell);
    uint8_t l = d1 >> 5; // level
    uint8_t h = (d1 & 16) == 16; // with hashes
    uint8_t s = (d1 & 8) == 8; // exotic
    uint8_t r = d1 & 7;	// refs count
    uint8_t absent = r == 7 && h;
    UNUSED(l);
    UNUSED(h);
    UNUSED(absent);

    VALIDATE(!s, ERR_INVALID_DATA); // only ordinary cells are valid
    VALIDATE(r <= MAX_REFERENCES_COUNT, ERR_INVALID_DATA);

    uint8_t data_size = Cell_get_data_size(cell);
    uint8_t refs_count = 0;
    uint8_t* refs = Cell_get_refs(cell, &refs_count);
    for (uint8_t i = 0; i < refs_count; ++i) {
        uint8_t ref = refs[i];
        VALIDATE(ref <= cells_count && ref > cell_index, ERR_INVALID_DATA);
    }

    return CELL_DATA_OFFSET + data_size + refs_count; // cell size
}

void deserialize_array(uint8_t* cell_data, uint8_t cell_data_size, uint16_t offset, uint8_t out_size, uint8_t* out) {
    uint8_t shift = offset % 8;
    uint8_t first_data_byte = offset / 8;
    for (uint16_t i = first_data_byte, j = 0; j < out_size; ++i, ++j) {
        VALIDATE(i == (j + first_data_byte) && (i + 1) < cell_data_size, ERR_INVALID_DATA);

        uint8_t cur = cell_data[i] << shift;
        out[j] = cur;

        if (j == out_size - 1) {
            out[j] |= cell_data[i + 1] >> (8 - shift);
        }

        if (i != first_data_byte) {
            out[j - 1] |= cell_data[i] >> (8 - shift);
        }
    }
}

void deserialize_address(struct SliceData_t* slice, uint8_t* cell_data, uint8_t cell_data_size) {
    {
        uint8_t f = SliceData_get_next_bit(slice);
        uint8_t ihr_disabled = SliceData_get_next_bit(slice);
        uint8_t bounce = SliceData_get_next_bit(slice);
        uint8_t bounced = SliceData_get_next_bit(slice);
        UNUSED(f);
        UNUSED(ihr_disabled);
        UNUSED(bounce);
        UNUSED(bounced);

        uint64_t use_src_address = SliceData_get_next_int(slice, 2);
        VALIDATE(use_src_address == 0, ERR_INVALID_DATA);

        uint64_t use_dst_address = SliceData_get_next_int(slice, 2);
        VALIDATE(use_dst_address == 2, ERR_INVALID_DATA);
        uint8_t anycast = SliceData_get_next_bit(slice);
        UNUSED(anycast);
    }

    int8_t wc = SliceData_get_next_byte(slice);
    uint16_t offset = SliceData_get_cursor(slice);
    uint8_t address[ADDRESS_LENGTH];
    deserialize_array(cell_data, cell_data_size, offset, ADDRESS_LENGTH, address);
    SliceData_move_by(slice, ADDRESS_LENGTH * 8);

    char* address_str = data_context.sign_tm_context.address_str;
    char wc_temp[6]; // snprintf always returns zero
    snprintf(wc_temp, sizeof(wc_temp), "%d:", wc);
    int wc_len = strlen(wc_temp);
    os_memcpy(address_str, wc_temp, wc_len);
    address_str += wc_len;
    snprintf(address_str, sizeof(data_context.sign_tm_context.address_str) - wc_len, "%.*h", sizeof(address), address);
}

void deserialize_amount(struct SliceData_t* slice, uint8_t* cell_data, uint8_t cell_data_size) {
    uint8_t amount_length = SliceData_get_next_int(slice, 4);
    VALIDATE(amount_length && amount_length <= MAX_AMOUNT_LENGTH, ERR_INVALID_DATA);

    uint16_t offset = SliceData_get_cursor(slice);
    uint8_t amount[MAX_AMOUNT_LENGTH];
    os_memset(amount, 0, sizeof(amount));
    deserialize_array(cell_data, cell_data_size, offset, amount_length, amount);
    SliceData_move_by(slice, amount_length * 8);

    char* amount_str = data_context.sign_tm_context.amount_str;
    uint8_t text_size = convert_hex_amount_to_displayable(amount, amount_length, amount_str);
    strcpy(amount_str + text_size, " TON");
}

void get_cell_hash(Cell_t* cell, uint8_t* hash, const uint8_t cell_index) {
    uint8_t hash_buffer[262]; // d1(1) + d2(1) + data(128) + 4 * (depth(1) + hash(32))

    uint16_t hash_buffer_offset = 0;
    hash_buffer[0] = Cell_get_d1(cell);
    hash_buffer[1] = Cell_get_d2(cell);
    hash_buffer_offset += 2;
    uint8_t data_size = Cell_get_data_size(cell);
    os_memcpy(hash_buffer + hash_buffer_offset, Cell_get_data(cell), data_size);
    hash_buffer_offset += data_size;

    uint8_t refs_count = 0;
    uint8_t* refs = Cell_get_refs(cell, &refs_count);
    VALIDATE(refs_count >= 0 && refs_count <= MAX_REFERENCES_COUNT, ERR_INVALID_DATA);
    for (uint8_t child = 0; child < refs_count; ++child) {
        uint8_t* depth = &message_context.cell_depth[cell_index];
        uint8_t child_depth = message_context.cell_depth[refs[child]];
        *depth = (*depth > child_depth + 1) ? *depth : (child_depth + 1);
        uint8_t buf[2];
        buf[0] = 0;
        buf[1] = child_depth;
        os_memcpy(hash_buffer + hash_buffer_offset, buf, sizeof(buf));
        hash_buffer_offset += sizeof(buf);
    }
    
    for (uint8_t child = 0; child < refs_count; ++child) {
        uint8_t* cell_hash = message_context.cells[refs[child]].hash;
        os_memcpy(hash_buffer + hash_buffer_offset, cell_hash, HASH_SIZE);
        hash_buffer_offset += HASH_SIZE;
    }
    
    int result = cx_hash_sha256(hash_buffer, hash_buffer_offset, hash, HASH_SIZE);
    VALIDATE(result == HASH_SIZE, ERR_INVALID_HASH);
}

void deserialize_message(struct ByteStream_t* src) {
    {
        uint32_t magic = ByteStream_read_u32(src);
        VALIDATE(BOC_GENERIC_TAG == magic, ERR_INVALID_DATA);
    }
    
    uint8_t first_byte = ByteStream_read_byte(src);
    {
        bool index_included = (first_byte & 0x80) != 0;
        bool has_crc = (first_byte & 0x40) != 0;
        bool has_cache_bits = (first_byte & 0x20) != 0;
        uint8_t flags = (first_byte & 0x18) >> 3;
        UNUSED(index_included);
        UNUSED(has_crc);
        UNUSED(has_cache_bits);
        UNUSED(flags);
    }

    uint8_t ref_size = first_byte & 0x7; // size in bytes
    VALIDATE(ref_size == 1, ERR_INVALID_DATA);
    
    uint8_t offset_size = ByteStream_read_byte(src);
    VALIDATE(offset_size != 0 && offset_size <= 8, ERR_INVALID_DATA);

    uint8_t cells_count = ByteStream_read_byte(src);
    uint8_t roots_count = ByteStream_read_byte(src);
    VALIDATE(roots_count == MAX_ROOTS_COUNT, ERR_INVALID_DATA);
    VALIDATE(cells_count == MAX_CELLS_COUNT, ERR_INVALID_MESSAGE);

    {
        uint8_t absent_count = ByteStream_read_byte(src);
        VALIDATE(absent_count == 0, ERR_INVALID_DATA);
        uint8_t* total_cells_size = ByteStream_read_data(src, offset_size);
        UNUSED(total_cells_size);
        uint8_t root_index = ByteStream_read_byte(src);
        VALIDATE(root_index == 0, ERR_INVALID_DATA);
        for (uint8_t i = 0; i < cells_count; ++i) {
            ByteStream_read_byte(src); // size index
        }
    }

    Cell_t* cells = message_context.cells;
    for (uint8_t i = 0; i < cells_count; ++i) {
        uint8_t* cell_begin = ByteStream_get_cursor(src);
        Cell_init(cells + i, cell_begin);
        uint16_t offset = deserialize_cell(cells + i, i, cells_count);
        ByteStream_read_data(src, offset);
    }

    {
        uint8_t* cell_data = Cell_get_data(&cells[0]);
        uint8_t cell_data_size = Cell_get_data_size(&cells[0]);
        VALIDATE(cell_data_size == 13, ERR_INVALID_MESSAGE);
        uint8_t send_mode = cell_data[12];
        VALIDATE(send_mode == 3, ERR_INVALID_SEND_MODE);
    }

    uint8_t* cell_data = Cell_get_data(&cells[ADDRESS_CELL_INDEX]);
    uint8_t cell_data_size = Cell_get_data_size(&cells[ADDRESS_CELL_INDEX]);
    SliceData_t slice;
    SliceData_init(&slice, cell_data, cell_data_size);
    deserialize_address(&slice, cell_data, cell_data_size);
    deserialize_amount(&slice, cell_data, cell_data_size);

    for (int16_t i = cells_count - 1; i >= 0; --i) {
        Cell_t* cell = &cells[i];
        get_cell_hash(cell, cell->hash, i);
    }

    os_memcpy(data_context.sign_tm_context.to_sign, cells[0].hash, HASH_SIZE);
}
