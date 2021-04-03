#include "apdu_constants.h"
#include "utils.h"
#include "errors.h"

static uint8_t set_result_sign() {
    cx_ecfp_private_key_t privateKey;
    SignContext_t* context = &data_context.sign_context;

    BEGIN_TRY {
        TRY {
            get_private_key(context->account_number, &privateKey);
            cx_eddsa_sign(&privateKey, CX_LAST, CX_SHA512, context->to_sign, TO_SIGN_LENGTH, NULL, 0, context->hash, SIGN_LENGTH, NULL);
        } FINALLY {
            os_memset(&privateKey, 0, sizeof(privateKey));
        }
    }
    END_TRY;

    uint8_t tx = 0;
    G_io_apdu_buffer[tx++] = SIGN_LENGTH;
    os_memmove(G_io_apdu_buffer + tx, context->hash, SIGN_LENGTH);
    tx += SIGN_LENGTH;
    return tx;
}

UX_STEP_NOCB(
    ux_sign_flow_1_step,
    pnn,
    {
      &C_icon_certificate,
      "Sign",
      "message",
    });
UX_STEP_NOCB(
    ux_sign_flow_2_step,
    bnnn_paging,
    {
      .title = "Message hash",
      .text = data_context.sign_context.to_sign_str,
    });
UX_STEP_CB(
    ux_sign_flow_3_step,
    pbb,
    send_response(0, false),
    {
      &C_icon_crossmark,
      "Cancel",
      "signature",
    });
UX_STEP_CB(
    ux_sign_flow_4_step,
    pbb,
    send_response(set_result_sign(), true),
    {
      &C_icon_validate_14,
      "Sign",
      "message",
    });

UX_FLOW(ux_sign_flow,
    &ux_sign_flow_1_step,
    &ux_sign_flow_2_step,
    &ux_sign_flow_3_step,
    &ux_sign_flow_4_step
);

void handleSign(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    VALIDATE(p1 == 0 && p2 == 0, ERR_INVALID_REQUEST);
    SignContext_t* context = &data_context.sign_context;
    VALIDATE(dataLength == (sizeof(context->account_number) + sizeof(context->to_sign)), ERR_INVALID_REQUEST);

    context->account_number = readUint32BE(dataBuffer);
    memcpy(context->to_sign, dataBuffer + sizeof(context->account_number), TO_SIGN_LENGTH);
    snprintf(context->to_sign_str, sizeof(context->to_sign_str), "%.*h", sizeof(context->to_sign), context->to_sign);

    ux_flow_init(0, ux_sign_flow, NULL);
    *flags |= IO_ASYNCH_REPLY;
}
