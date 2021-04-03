#include "os.h"
#include "ux.h"
#include "utils.h"
#include "apdu_constants.h"
#include "errors.h"

static uint8_t set_result_get_public_key() {
    uint8_t tx = 0;
    G_io_apdu_buffer[tx++] = PUBLIC_KEY_LENGTH;
    os_memmove(G_io_apdu_buffer + tx, data_context.pk_context.public_key, PUBLIC_KEY_LENGTH);
    tx += PUBLIC_KEY_LENGTH;
    return tx;
}

UX_STEP_NOCB(
    ux_display_public_flow_1_step,
    bnnn_paging,
    {
        .title = "Public key",
        .text = data_context.pk_context.public_key_str,
    });
UX_STEP_CB(
    ux_display_public_flow_2_step,
    pb,
    send_response(0, false),
    {
        &C_icon_crossmark,
        "Reject",
    });
UX_STEP_CB(
    ux_display_public_flow_3_step,
    pb,
    send_response(set_result_get_public_key(), true),
    {
        &C_icon_validate_14,
        "Approve",
    });

UX_FLOW(ux_display_public_flow,
    &ux_display_public_flow_1_step,
    &ux_display_public_flow_2_step,
    &ux_display_public_flow_3_step
);

void handleGetPublicKey(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    VALIDATE(p2 == 0 && dataLength == sizeof(uint32_t), ERR_INVALID_REQUEST);

    uint32_t account_number = readUint32BE(dataBuffer);
    PublicKeyContext_t* context = &data_context.pk_context;
    get_public_key(account_number, context->public_key);
    if (p1 == P1_NON_CONFIRM) {
        *tx = set_result_get_public_key();
        THROW(SUCCESS);
    } 

    if (p1 == P1_CONFIRM) {
        snprintf(context->public_key_str, sizeof(context->public_key_str), "%.*h", sizeof(context->public_key), context->public_key);
        ux_flow_init(0, ux_display_public_flow, NULL);
        *flags |= IO_ASYNCH_REPLY;
        return;
    }

    THROW(ERR_INVALID_REQUEST);
}
