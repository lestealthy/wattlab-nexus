#include "nexus_decoders.h"
#include <string.h>

nexus_err_t nexus_decode_uart_frame(const uint8_t* bits, size_t bit_count,
                                    const nexus_uart_frame_cfg_t* cfg,
                                    uint8_t* out_byte, size_t* out_bits_used) {
    if (!bits || !cfg || !out_byte || !out_bits_used) return NEXUS_ERR_INVALID_PARAM;
    if (cfg->data_bits < 5 || cfg->data_bits > 9) return NEXUS_ERR_INVALID_PARAM;
    if (cfg->stop_bits < 1 || cfg->stop_bits > 2) return NEXUS_ERR_INVALID_PARAM;

    // bits[0] must be the start bit (logic 0)
    if (bits[0] != 0) return NEXUS_ERR_PROTOCOL;

    size_t idx = 1;
    uint8_t value = 0;
    for (uint8_t i = 0; i < cfg->data_bits; i++) {
        if (idx >= bit_count) return NEXUS_ERR_PROTOCOL;
        value |= (uint8_t)((bits[idx] & 1) << i); // LSB first
        idx++;
    }

    if (cfg->parity_enable) {
        if (idx >= bit_count) return NEXUS_ERR_PROTOCOL;
        uint8_t parity_bit = bits[idx] & 1;
        uint8_t ones = 0;
        for (uint8_t i = 0; i < cfg->data_bits; i++) ones += (value >> i) & 1;
        uint8_t expected = cfg->parity_odd ? ((ones % 2 == 0) ? 1 : 0)
                                           : ((ones % 2 == 0) ? 0 : 1);
        if (parity_bit != expected) return NEXUS_ERR_PROTOCOL;
        idx++;
    }

    for (uint8_t s = 0; s < cfg->stop_bits; s++) {
        if (idx >= bit_count) return NEXUS_ERR_PROTOCOL;
        if (bits[idx] != 1) return NEXUS_ERR_PROTOCOL;
        idx++;
    }

    *out_byte = value;
    *out_bits_used = idx;
    return NEXUS_OK;
}

void nexus_i2c_txn_reset(nexus_i2c_transaction_t* txn) {
    if (!txn) return;
    memset(txn, 0, sizeof(*txn));
}

nexus_err_t nexus_i2c_txn_add(nexus_i2c_transaction_t* txn, nexus_i2c_op_t op, uint8_t value, bool ack) {
    if (!txn) return NEXUS_ERR_INVALID_PARAM;
    if (txn->count >= (uint8_t)(sizeof(txn->steps) / sizeof(txn->steps[0]))) {
        txn->overflow = true;
        return NEXUS_ERR_BUFFER_OVERFLOW;
    }
    txn->steps[txn->count].op = op;
    txn->steps[txn->count].value = value;
    txn->steps[txn->count].ack = ack;
    txn->count++;
    return NEXUS_OK;
}

nexus_err_t nexus_decode_i2c_transaction(nexus_i2c_transaction_t* txn) {
    if (!txn) return NEXUS_ERR_INVALID_PARAM;
    if (txn->count == 0) return NEXUS_ERR_PROTOCOL;
    // A well-formed transaction begins with START and ends with STOP.
    if (txn->steps[0].op != NEXUS_I2C_OP_START) return NEXUS_ERR_PROTOCOL;
    if (txn->steps[txn->count - 1].op != NEXUS_I2C_OP_STOP) return NEXUS_ERR_PROTOCOL;
    return NEXUS_OK;
}

nexus_err_t nexus_decode_spi_byte(uint8_t mosi_byte, uint8_t* miso_byte, const nexus_spi_transfer_t* cfg) {
    if (!miso_byte || !cfg) return NEXUS_ERR_INVALID_PARAM;
    // With no physical MISO line modelled here, echo the MOSI byte; a real
    // capture source supplies the sampled MISO bits instead.
    *miso_byte = mosi_byte;
    return NEXUS_OK;
}

nexus_err_t nexus_decode_can_standard(const uint8_t* bits, size_t bit_count, nexus_can_decoded_t* out) {
    if (!bits || !out) return NEXUS_ERR_INVALID_PARAM;
    // Minimal standard-frame decode: SOF + 11-bit ID + RTR + IDE + r0 + DLC.
    // Layout (CAN 2.0A), after the dominant SOF at bits[0]:
    //   bits[1..11]  : identifier (MSB first)
    //   bits[12]     : RTR
    //   bits[13]     : IDE (0 for standard)
    //   bits[14]     : r0
    //   bits[15..18] : DLC
    if (bit_count < 19) return NEXUS_ERR_PROTOCOL;

    uint32_t id = 0;
    for (int i = 0; i < 11; i++) {
        id = (id << 1) | (bits[1 + i] & 1);
    }

    bool rtr = bits[12] & 1;
    bool ide = bits[13] & 1;
    uint8_t dlc = 0;
    for (int i = 0; i < 4; i++) {
        dlc = (uint8_t)((dlc << 1) | (bits[15 + i] & 1));
    }

    if (ide) return NEXUS_ERR_NOT_SUPPORTED; // extended frame handled elsewhere
    if (dlc > 8) return NEXUS_ERR_PROTOCOL;

    memset(out, 0, sizeof(*out));
    out->id = id;
    out->extended = false;
    out->rtr = rtr;
    out->dlc = dlc;

    // Data field begins immediately after the DLC field (bit 19), MSB first.
    if (!rtr) {
        size_t pos = 19;
        for (uint8_t b = 0; b < dlc; b++) {
            if (pos + 8 > bit_count) return NEXUS_ERR_PROTOCOL;
            uint8_t byte = 0;
            for (int i = 0; i < 8; i++) {
                byte = (uint8_t)((byte << 1) | (bits[pos + i] & 1));
            }
            out->data[b] = byte;
            pos += 8;
        }
    }

    return NEXUS_OK;
}
