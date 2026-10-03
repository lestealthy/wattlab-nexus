#ifndef NEXUS_DECODERS_H
#define NEXUS_DECODERS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nexus_errors.h"

/*
 * Pure protocol decoders. These are deliberately hardware-independent and
 * allocation-free so they can be unit tested on the host and reused on the
 * device. They operate on already-captured edge/byte streams.
 */

// ---- UART framing -----------------------------------------------------------
typedef struct {
    uint32_t baud;
    uint8_t data_bits;
    bool parity_enable;
    bool parity_odd;
    uint8_t stop_bits;
} nexus_uart_frame_cfg_t;

// Decode a single UART frame from a sampled bit sequence.
// bits[0] is the start bit; samples_per_bit is the oversampling rate.
nexus_err_t nexus_decode_uart_frame(const uint8_t* bits, size_t bit_count,
                                    const nexus_uart_frame_cfg_t* cfg,
                                    uint8_t* out_byte, size_t* out_bits_used);

// ---- I2C transactions -------------------------------------------------------
typedef enum {
    NEXUS_I2C_OP_START = 0,
    NEXUS_I2C_OP_ADDR_W,
    NEXUS_I2C_OP_ADDR_R,
    NEXUS_I2C_OP_DATA,
    NEXUS_I2C_OP_ACK,
    NEXUS_I2C_OP_NACK,
    NEXUS_I2C_OP_STOP,
} nexus_i2c_op_t;

typedef struct {
    nexus_i2c_op_t op;
    uint8_t value;   // address (7-bit, unshifted) or data byte
    bool ack;        // meaningful for ADDR/DATA
} nexus_i2c_step_t;

typedef struct {
    nexus_i2c_step_t steps[64];
    uint8_t count;
    bool overflow;
} nexus_i2c_transaction_t;

nexus_err_t nexus_decode_i2c_transaction(nexus_i2c_transaction_t* txn);
nexus_err_t nexus_i2c_txn_add(nexus_i2c_transaction_t* txn, nexus_i2c_op_t op, uint8_t value, bool ack);
void       nexus_i2c_txn_reset(nexus_i2c_transaction_t* txn);

// ---- SPI transfers ----------------------------------------------------------
typedef struct {
    uint8_t mosi[64];
    uint8_t miso[64];
    uint8_t len;
    bool cpola;   // CPOL
    bool cpha;    // CPHA
    bool msb_first;
} nexus_spi_transfer_t;

nexus_err_t nexus_decode_spi_byte(uint8_t mosi_byte, uint8_t* miso_byte, const nexus_spi_transfer_t* cfg);

// ---- CAN frames -------------------------------------------------------------
typedef struct {
    uint32_t id;
    bool extended;
    bool rtr;
    uint8_t dlc;
    uint8_t data[8];
} nexus_can_decoded_t;

// Decode a standard (11-bit) CAN frame from a bit sequence.
nexus_err_t nexus_decode_can_standard(const uint8_t* bits, size_t bit_count, nexus_can_decoded_t* out);

#endif
