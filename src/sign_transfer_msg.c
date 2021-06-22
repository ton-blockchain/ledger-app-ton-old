#include "apdu_constants.h"
#include "utils.h"
#include "errors.h"
#include "byte_stream.h"
#include "message.h"

static uint8_t set_result_sign_transfer_msg() {
    cx_ecfp_private_key_t privateKey;
    SignTransferMsgContext_t* context = &data_context.sign_tm_context;

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
    ux_sign_transfer_msg_flow_1_step,
    pnn,
    {
      &C_icon_eye,
      "Review",
      "transaction",
    });
UX_STEP_NOCB(
    ux_sign_transfer_msg_flow_2_step,
    bnnn_paging,
    {
      .title = "Amount",
      .text = data_context.sign_tm_context.amount_str,
    });
UX_STEP_NOCB(
    ux_sign_transfer_msg_flow_3_step,
    bnnn_paging,
    {
      .title = "Address",
      .text = data_context.sign_tm_context.address_str,
    });
UX_STEP_CB(
    ux_sign_transfer_msg_flow_4_step,
    pbb,
    send_response(set_result_sign_transfer_msg(), true),
    {
      &C_icon_validate_14,
      data_context.sign_tm_context.send_text,
      "and send",
    });
UX_STEP_CB(
    ux_sign_transfer_msg_flow_5_step,
    pb,
    send_response(0, false),
    {
      &C_icon_crossmark,
      "Reject",
    });

UX_FLOW(ux_sign_transfer_msg_flow,
    &ux_sign_transfer_msg_flow_1_step,
    &ux_sign_transfer_msg_flow_2_step,
    &ux_sign_transfer_msg_flow_3_step,
    &ux_sign_transfer_msg_flow_4_step,
    &ux_sign_transfer_msg_flow_5_step
);

void handleSignTransferMsg(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    VALIDATE(p2 == 0 && dataLength > sizeof(uint32_t), ERR_INVALID_REQUEST);
    SignTransferMsgContext_t* context = &data_context.sign_tm_context;

    context->account_number = readUint32BE(dataBuffer);
    uint8_t* msg_begin = dataBuffer + sizeof(context->account_number);
    uint8_t msg_length = dataLength - sizeof(context->account_number);

    ByteStream_t src;
    ByteStream_init(&src, msg_begin, msg_length);
    uint8_t display_flags = p1;
    deserialize_message(&src, display_flags);

    ux_flow_init(0, ux_sign_transfer_msg_flow, NULL);
    *flags |= IO_ASYNCH_REPLY;
}
