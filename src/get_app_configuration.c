#include "apdu_constants.h"
#include "utils.h"
#include "errors.h"

void handleGetAppConfiguration(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    UNUSED(p1);
    UNUSED(p2);
    UNUSED(dataBuffer);
    UNUSED(dataLength);
    UNUSED(flags);
    VALIDATE(p1 == 0 && p2 == 0 && dataLength == 0, ERR_INVALID_REQUEST);

    G_io_apdu_buffer[0] = LEDGER_MAJOR_VERSION;
    G_io_apdu_buffer[1] = LEDGER_MINOR_VERSION;
    G_io_apdu_buffer[2] = LEDGER_PATCH_VERSION;
    *tx = 3;
    THROW(SUCCESS);
}
