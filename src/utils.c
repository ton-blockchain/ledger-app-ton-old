#include "os.h"
#include "cx.h"
#include "utils.h"
#include "menu.h"

#include <stdlib.h>

static cx_ecfp_public_key_t publicKey;
void get_public_key(uint32_t account_number, uint8_t* publicKeyArray) {
    cx_ecfp_private_key_t privateKey;

    cx_ecfp_init_public_key(CX_CURVE_Ed25519, NULL, 0, &publicKey);
    BEGIN_TRY {
        TRY {
            get_private_key(account_number, &privateKey);
            cx_ecfp_generate_pair(CX_CURVE_Ed25519, &publicKey, &privateKey, 1);
            io_seproxyhal_io_heartbeat();
        } FINALLY {
            os_memset(&privateKey, 0, sizeof(privateKey));
        }
    }
    END_TRY;

    for (int i = 0; i < 32; i++) {
        publicKeyArray[i] = publicKey.W[64 - i];
    }
    if ((publicKey.W[32] & 1) != 0) {
        publicKeyArray[31] |= 0x80;
    }
}

uint32_t readUint32BE(uint8_t *buffer) {
  return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | (buffer[3]);
}

static const uint32_t HARDENED_OFFSET = 0x80000000;

void get_private_key(uint32_t account_number, cx_ecfp_private_key_t *privateKey) {
    const uint32_t derivePath[BIP32_PATH] = {
            44 | HARDENED_OFFSET,
            607 | HARDENED_OFFSET,
            account_number | HARDENED_OFFSET,
            0 | HARDENED_OFFSET,
            0 | HARDENED_OFFSET
    };

    uint8_t privateKeyData[32];
    BEGIN_TRY {
        TRY {
            io_seproxyhal_io_heartbeat();
            os_perso_derive_node_bip32_seed_key(HDW_ED25519_SLIP10, CX_CURVE_Ed25519, derivePath, BIP32_PATH, privateKeyData, NULL, NULL, 0);
            cx_ecfp_init_private_key(CX_CURVE_Ed25519, privateKeyData, 32, privateKey);
            io_seproxyhal_io_heartbeat();
        } FINALLY {
            os_memset(privateKeyData, 0, sizeof(privateKeyData));
        }
    }
    END_TRY;
}

void send_response(uint8_t tx, bool approve) {
    G_io_apdu_buffer[tx++] = approve? 0x90 : 0x69;
    G_io_apdu_buffer[tx++] = approve? 0x00 : 0x85;
    reset_app_context();
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
    // Display back the original UX
    ui_idle();
}

unsigned int ui_prepro(const bagl_element_t *element) {
    unsigned int display = 1;
    if (element->component.userid > 0) {
        display = (ux_step == element->component.userid - 1);
        if (display) {
            if (element->component.userid == 1) {
                UX_CALLBACK_SET_INTERVAL(2000);
            }
            else {
                UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
            }
        }
    }
    return display;
}

uint8_t leading_zeros(uint16_t value) {
    uint8_t lz = 0;
    uint16_t msb = 0x8000;
    for (uint8_t i = 0; i < 16; ++i) {
        if ((value << i) & msb) {
            break;
        }
        ++lz;
    }

    return lz;
}

#define SCRATCH_SIZE 37
uint8_t convert_hex_amount_to_displayable(uint8_t* amount, uint8_t amount_length, char* out) {
    uint8_t LOOP1 = 28;
    uint8_t LOOP2 = 9;
    uint16_t scratch[SCRATCH_SIZE];
    uint8_t offset = 0;
    uint8_t nonZero = 0;
    uint8_t i;
    uint8_t targetOffset = 0;
    uint8_t workOffset;
    uint8_t j;
    uint8_t nscratch = SCRATCH_SIZE;
    uint8_t smin = nscratch - 2;
    uint8_t comma = 0;

    for (i = 0; i < SCRATCH_SIZE; i++) {
        scratch[i] = 0;
    }
    for (i = 0; i < amount_length; i++) {
        for (j = 0; j < 8; j++) {
            uint8_t k;
            uint16_t shifted_in =
                (((amount[i] & 0xff) & ((1 << (7 - j)))) != 0) ? (short)1
                                                               : (short)0;
            for (k = smin; k < nscratch; k++) {
                scratch[k] += ((scratch[k] >= 5) ? 3 : 0);
            }
            if (scratch[smin] >= 8) {
                smin -= 1;
            }
            for (k = smin; k < nscratch - 1; k++) {
                scratch[k] =
                    ((scratch[k] << 1) & 0xF) | ((scratch[k + 1] >= 8) ? 1 : 0);
            }
            scratch[nscratch - 1] = ((scratch[nscratch - 1] << 1) & 0x0F) |
                                    (shifted_in == 1 ? 1 : 0);
        }
    }

    for (i = 0; i < LOOP1; i++) {
        if (!nonZero && (scratch[offset] == 0)) {
            offset++;
        } else {
            nonZero = 1;
            out[targetOffset++] = scratch[offset++] + '0';
        }
    }
    if (targetOffset == 0) {
        out[targetOffset++] = '0';
    }
    workOffset = offset;
    for (i = 0; i < LOOP2; i++) {
        unsigned char allZero = 1;
        unsigned char j;
        for (j = i; j < LOOP2; j++) {
            if (scratch[workOffset + j] != 0) {
                allZero = 0;
                break;
            }
        }
        if (allZero) {
            break;
        }
        if (!comma) {
            out[targetOffset++] = '.';
            comma = 1;
        }
        out[targetOffset++] = scratch[offset++] + '0';
    }
    return targetOffset;
}

uint16_t crc16(char *ptr, int count) {
    int crc = 0;
    uint8_t i;
    while (--count >= 0) {
        crc = crc ^ (int)*ptr++ << 8;
        i = 8;
        do {
            if (crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
        } while (--i);
    }
    return (crc);
}

static const char base64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const int base64_mod_table[] = {0, 2, 1};

size_t base64_encode(const uint8_t *data, int data_length, char *out) {
    size_t out_length = 4 * ((data_length + 2) / 3);
    for (int i = 0, j = 0; i < data_length;) {
        uint32_t octet_a = i < data_length ? data[i++] : 0;
        uint32_t octet_b = i < data_length ? data[i++] : 0;
        uint32_t octet_c = i < data_length ? data[i++] : 0;
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        out[j++] = base64_alphabet[(triple >> 3 * 6) & 0x3F];
        out[j++] = base64_alphabet[(triple >> 2 * 6) & 0x3F];
        out[j++] = base64_alphabet[(triple >> 1 * 6) & 0x3F];
        out[j++] = base64_alphabet[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < base64_mod_table[data_length % 3]; i++) {
        out[out_length - 1 - i] = '=';
    }

    out[out_length] = '\0';
    return out_length;
}
