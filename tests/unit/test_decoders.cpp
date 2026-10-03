#include <stdio.h>
#include <string.h>
#include "nexus_decoders.h"
#include "nexus_errors.h"

static int test_uart(void) {
    // LSB-first frame for byte 0x55 at 8N1:
    // start=0, then bits 1,0,1,0,1,0,1,0, then stop=1
    uint8_t bits[] = {0, 1,0,1,0,1,0,1,0, 1};
    nexus_uart_frame_cfg_t cfg = {115200, 8, false, false, 1};
    uint8_t out = 0; size_t used = 0;
    nexus_err_t err = nexus_decode_uart_frame(bits, sizeof(bits), &cfg, &out, &used);
    if (err != NEXUS_OK) { printf("  FAIL: uart decode err=%d\n", err); return 1; }
    if (out != 0x55)     { printf("  FAIL: uart byte=0x%02X expected 0x55\n", out); return 1; }
    if (used != 10)      { printf("  FAIL: uart bits_used=%zu expected 10\n", used); return 1; }
    printf("  PASS: UART frame decode 0x55\n");

    // Bad stop bit must be rejected
    uint8_t bad[] = {0, 1,0,1,0,1,0,1,0, 0};
    err = nexus_decode_uart_frame(bad, sizeof(bad), &cfg, &out, &used);
    if (err != NEXUS_ERR_PROTOCOL) { printf("  FAIL: bad stop accepted\n"); return 1; }
    printf("  PASS: UART bad stop rejected\n");

    // Invalid start bit
    uint8_t no_start[] = {1, 0,0,0,0,0,0,0,0, 1};
    err = nexus_decode_uart_frame(no_start, sizeof(no_start), &cfg, &out, &used);
    if (err != NEXUS_ERR_PROTOCOL) { printf("  FAIL: missing start accepted\n"); return 1; }
    printf("  PASS: UART missing start rejected\n");
    return 0;
}

static int test_i2c(void) {
    nexus_i2c_transaction_t txn;
    nexus_i2c_txn_reset(&txn);
    nexus_i2c_txn_add(&txn, NEXUS_I2C_OP_START, 0, false);
    nexus_i2c_txn_add(&txn, NEXUS_I2C_OP_ADDR_W, 0x68, true);
    nexus_i2c_txn_add(&txn, NEXUS_I2C_OP_DATA, 0x75, true);
    nexus_i2c_txn_add(&txn, NEXUS_I2C_OP_STOP, 0, false);
    if (nexus_decode_i2c_transaction(&txn) != NEXUS_OK) { printf("  FAIL: i2c txn decode\n"); return 1; }
    if (txn.count != 4) { printf("  FAIL: i2c count=%u\n", txn.count); return 1; }
    printf("  PASS: I2C transaction decode\n");

    // Missing START should fail
    nexus_i2c_transaction_t bad;
    nexus_i2c_txn_reset(&bad);
    nexus_i2c_txn_add(&bad, NEXUS_I2C_OP_DATA, 0x01, true);
    nexus_i2c_txn_add(&bad, NEXUS_I2C_OP_STOP, 0, false);
    if (nexus_decode_i2c_transaction(&bad) != NEXUS_ERR_PROTOCOL) { printf("  FAIL: i2c missing start accepted\n"); return 1; }
    printf("  PASS: I2C missing START rejected\n");

    // Overflow protection
    nexus_i2c_transaction_t big;
    nexus_i2c_txn_reset(&big);
    for (int i = 0; i < 100; i++) nexus_i2c_txn_add(&big, NEXUS_I2C_OP_DATA, 0, true);
    if (!big.overflow) { printf("  FAIL: i2c overflow not flagged\n"); return 1; }
    printf("  PASS: I2C overflow flagged\n");
    return 0;
}

static int test_can(void) {
    // Build a standard CAN frame: ID=0x123, DLC=1, data=0xAB.
    // bits[0]=SOF(0), bits[1..11]=ID (MSB first), bit12=RTR(0),
    // bit13=IDE(0), bit14=r0(0), bits15..18=DLC=0001, then 8 data bits.
    uint8_t bits[32];
    int n = 0;
    bits[n++] = 0; // SOF dominant
    uint32_t id = 0x123;
    for (int i = 10; i >= 0; i--) bits[n++] = (id >> i) & 1;
    bits[n++] = 0; // RTR
    bits[n++] = 0; // IDE
    bits[n++] = 0; // r0
    // DLC = 1
    bits[n++] = 0; bits[n++] = 0; bits[n++] = 0; bits[n++] = 1;
    // data 0xAB = 10101011
    uint8_t d = 0xAB;
    for (int i = 7; i >= 0; i--) bits[n++] = (d >> i) & 1;

    nexus_can_decoded_t frame;
    nexus_err_t err = nexus_decode_can_standard(bits, (size_t)n, &frame);
    if (err != NEXUS_OK) { printf("  FAIL: can decode err=%d\n", err); return 1; }
    if (frame.id != 0x123) { printf("  FAIL: can id=0x%X\n", frame.id); return 1; }
    if (frame.dlc != 1)    { printf("  FAIL: can dlc=%u\n", frame.dlc); return 1; }
    if (frame.data[0] != 0xAB) { printf("  FAIL: can data=0x%02X\n", frame.data[0]); return 1; }
    printf("  PASS: CAN standard frame decode\n");
    return 0;
}

int test_decoders(void) {
    printf("\n");
    if (test_uart()) return 1;
    if (test_i2c()) return 1;
    if (test_can()) return 1;
    return 0;
}
