#include "apdu_constants.h"
#include "globals.h"
#include "utils.h"
#include "wallet.h"
#include "byte_stream.h"

static uint8_t set_result_get_address() {
    uint8_t tx = 0;
    G_io_apdu_buffer[tx++] = ADDRESS_LENGTH;
    os_memmove(G_io_apdu_buffer + tx, data_context.addr_context.address, ADDRESS_LENGTH);
    tx += ADDRESS_LENGTH;
    return tx;
}

UX_STEP_NOCB(
    ux_display_address_flow_1_step,
    pnn,
    {
      &C_icon_eye,
      "Verify",
      "address",
    });
UX_STEP_NOCB(
    ux_display_address_flow_2_step, 
    bnnn_paging, 
    {
      .title = "Address",
      .text = data_context.addr_context.address_str,
    });
UX_STEP_CB(
    ux_display_address_flow_3_step, 
    pb, 
    send_response(0, false),
    {
      &C_icon_crossmark,
      "Reject",
    });
UX_STEP_CB(
    ux_display_address_flow_4_step, 
    pb, 
    send_response(set_result_get_address(), true),
    {
      &C_icon_validate_14,
      "Approve",
    });

UX_FLOW(ux_display_address_flow,
    &ux_display_address_flow_1_step,
    &ux_display_address_flow_2_step,
    &ux_display_address_flow_3_step,
    &ux_display_address_flow_4_step
);

void handleGetAddress(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    VALIDATE(p2 == 0 && dataLength == sizeof(uint32_t), ERR_INVALID_REQUEST);

    const uint32_t account_number = readUint32BE(dataBuffer);
    get_address(account_number, data_context.addr_context.address);

    if (p1 == P1_NON_CONFIRM) {
        *tx = set_result_get_address();
        THROW(SUCCESS);
    } 
    if (p1 == P1_CONFIRM) {
        AddressContext_t* context = &data_context.addr_context;
        snprintf(context->address_str, sizeof(context->address_str), "%.*h", sizeof(context->address), context->address);
        ux_flow_init(0, ux_display_address_flow, NULL);
        *flags |= IO_ASYNCH_REPLY;
        return;
    }

    THROW(ERR_INVALID_REQUEST);
}
