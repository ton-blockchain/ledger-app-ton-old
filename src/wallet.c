#include "wallet.h"
#include "slice_data.h"
#include "byte_stream.h"
#include "cell.h"
#include "utils.h"
#include "errors.h"

#define PUBLIC_KEY_CELL_INDEX 2

uint8_t deserialize_cells_tree(struct ByteStream_t* src) {
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
    VALIDATE(cells_count <= MAX_CELLS_COUNT, ERR_INVALID_DATA);

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

    Cell_t* cells = boc_context.cells;
    for (uint8_t i = 0; i < cells_count; ++i) {
        uint8_t* cell_begin = ByteStream_get_cursor(src);
        Cell_init(cells + i, cell_begin);
        uint16_t offset = deserialize_cell(cells + i, i, cells_count);
        ByteStream_read_data(src, offset);
    }

    return cells_count;
}

const uint8_t wallet[] = {
    0xB5,0xEE,0x9C,0x72,0xC1,0x01,0x03,0x01,0x00,0x91,0x00,0x00,0x05,0x67,0x02,0x01,0x34,0x01,0x02,0x00,0xC0,0xFF,0x00,0x20,0xDD,0x20,0x82,0x01,0x4C,0x97,0xBA,0x97,
    0x30,0xED,0x44,0xD0,0xD7,0x0B,0x1F,0xE0,0xA4,0xF2,0x60,0x83,0x08,0xD7,0x18,0x20,0xD3,0x1F,0xD3,0x1F,0xD3,0x1F,0xF8,0x23,0x13,0xBB,0xF2,0x63,0xED,0x44,0xD0,0xD3,
    0x1F,0xD3,0x1F,0xD3,0xFF,0xD1,0x51,0x32,0xBA,0xF2,0xA1,0x51,0x44,0xBA,0xF2,0xA2,0x04,0xF9,0x01,0x54,0x10,0x55,0xF9,0x10,0xF2,0xA3,0xF8,0x00,0x93,0x20,0xD7,0x4A,
    0x96,0xD3,0x07,0xD4,0x02,0xFB,0x00,0xE8,0xD1,0x01,0xA4,0xC8,0xCB,0x1F,0xCB,0x1F,0xCB,0xFF,0xC9,0xED,0x54,0x00,0x50,0x00,0x00,0x00,0x00,0x29,0xA9,0xA3,0x17,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00
};

void get_address(const uint32_t account_number, uint8_t* address) {
    ByteStream_t src;
    ByteStream_init(&src, (uint8_t*)wallet, sizeof(wallet));
    uint8_t cells_count = deserialize_cells_tree(&src);
    VALIDATE(cells_count == MAX_WALLET_CELLS_COUNT, ERR_INVALID_DATA);

    get_public_key(account_number, data_context.pk_context.public_key);
    Cell_t* cells = boc_context.cells;
    Cell_t* pk_cell = &cells[PUBLIC_KEY_CELL_INDEX];
    uint8_t pk_cell_size = Cell_get_data_size(pk_cell) + CELL_DATA_OFFSET;
    VALIDATE(pk_cell_size >= PUBLIC_KEY_LENGTH + CELL_DATA_OFFSET, ERR_INVALID_DATA);

    uint8_t temp_data[128];
    os_memcpy(temp_data, pk_cell->cell_begin, pk_cell_size - PUBLIC_KEY_LENGTH);
    os_memcpy(temp_data + pk_cell_size - PUBLIC_KEY_LENGTH, data_context.pk_context.public_key, PUBLIC_KEY_LENGTH);
    pk_cell->cell_begin = temp_data;

    for (int16_t i = cells_count - 1; i >= 0; --i) {
        Cell_get_hash(&cells[i], i);
    }

    os_memcpy(address, Cell_get_hash(boc_context.cells, 0), ADDRESS_LENGTH);
}

static const uint8_t bounceable_tag = 0x11;
static const uint8_t non_bounceable_tag = 0x51;
static const uint8_t test_flag = 0x80;
void address_to_string(int8_t wc, uint8_t display_flags, uint8_t* address, uint8_t in_len, uint8_t out_len, char* str) {
    VALIDATE(in_len == ADDRESS_LENGTH && out_len >= 70, ERR_INVALID_DATA);

    bool is_user_friendly = display_flags & USER_FRIENDLY;
    bool is_url_safe = display_flags & URL_SAFE;
    bool is_bounceable = display_flags & BOUNCEABLE;
    bool is_test_only = display_flags & TEST_ONLY;
    
    if (is_user_friendly) {
        uint8_t tag = is_bounceable ? bounceable_tag : non_bounceable_tag;
        if (is_test_only) {
            tag |= test_flag;
        }

        uint8_t addr[36];
        addr[0] = tag;
        addr[1] = wc;
        os_memcpy(addr + 2, address, in_len);

        uint8_t crc_offset = in_len + 2;
        uint16_t crc = crc16((char*)addr, crc_offset);
        addr[crc_offset] = crc >> 8;
        addr[crc_offset + 1] = crc & 0xFF;

        size_t out_len = base64_encode(addr, sizeof(addr), str);

        if (is_url_safe) {
            for (uint8_t i = 0; i < out_len; ++i) {
                if (str[i] == '+') {
                    str[i] = '-';
                }
                if (str[i] == '/') {
                    str[i] = '_';
                }
            }
        }
    }
    else {
        char wc_temp[6]; // snprintf always returns zero
        snprintf(wc_temp, sizeof(wc_temp), "%d:", wc);
        int wc_len = strlen(wc_temp);
        os_memcpy(str, wc_temp, wc_len);
        snprintf(str + wc_len, out_len - wc_len, "%.*h", in_len, address);
    }
}
