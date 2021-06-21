#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include "os.h"
#include "ux.h"
#include "os_io_seproxyhal.h"
#include "errors.h"
#include "cell.h"

#define P1_CONFIRM 0x01
#define P1_NON_CONFIRM 0x00

#define PUBLIC_KEY_LENGTH 32
#define ADDRESS_LENGTH 32
#define BIP32_PATH 5
#define TO_SIGN_LENGTH 32
#define SIGN_LENGTH 64
#define MAX_AMOUNT_LENGTH 0xF
#define HASH_SIZE 32
#define BOC_GENERIC_TAG 0xb5ee9c72
#define MAX_ROOTS_COUNT 1
#define MAX_CELLS_COUNT 16
#define MAX_MESSAGE_CELLS_COUNT 2
#define MAX_WALLET_CELLS_COUNT 3

extern ux_state_t ux;
// display stepped screens
extern unsigned int ux_step;
extern unsigned int ux_step_count;

void reset_app_context(void);

typedef struct AddressContext_t {
    uint8_t address[ADDRESS_LENGTH];
    char address_str[70];
} AddressContext_t;

typedef struct PublicKeyContext_t {
    uint8_t public_key[PUBLIC_KEY_LENGTH];
    char public_key_str[65];
} PublicKeyContext_t;

typedef struct SignContext_t {
    uint8_t to_sign[TO_SIGN_LENGTH];
    uint8_t hash[SIGN_LENGTH];
    uint32_t account_number;
    char to_sign_str[65];
} SignContext_t;

typedef struct SignTransferMsgContext_t {
    uint8_t to_sign[TO_SIGN_LENGTH];
    uint8_t hash[SIGN_LENGTH];
    char address_str[70];
    char amount_str[40];
    uint32_t account_number;
} SignTransferMsgContext_t;

typedef union {
    PublicKeyContext_t pk_context;
    AddressContext_t addr_context;
    SignContext_t sign_context;
    SignTransferMsgContext_t sign_tm_context;
} DataContext_t;

typedef struct BocContext_t {
    Cell_t cells[MAX_CELLS_COUNT];
    uint8_t cell_depth[MAX_CELLS_COUNT];
} BocContext_t;

extern DataContext_t data_context;
extern BocContext_t boc_context;

#endif
