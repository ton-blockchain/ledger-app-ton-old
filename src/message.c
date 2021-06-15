#include "message.h"
#include "utils.h"
#include "error.h"
#include "slice_data.h"
#include "byte_stream.h"
#include "wallet.h"

#define ADDRESS_CELL_INDEX 1

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

void deserialize_message(struct ByteStream_t* src) {
    uint8_t cells_count = deserialize_cells_tree(src);
    VALIDATE(cells_count == MAX_MESSAGE_CELLS_COUNT, ERR_INVALID_MESSAGE);
    Cell_t* cells = boc_context.cells;

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

    for (int16_t i = cells_count - 1; i > 0; --i) {
        Cell_get_hash(&cells[i], i);
    }

    os_memcpy(data_context.sign_tm_context.to_sign, Cell_get_hash(cells, 0), HASH_SIZE);
}
