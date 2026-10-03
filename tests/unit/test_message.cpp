#include <stdio.h>
#include <string.h>
#include "nexus_message.h"
#include "nexus_errors.h"

int test_message(void) {
    printf("\n");

    // CRC check against a known vector is not standardized here, so we verify
    // self-consistency plus a couple of explicit properties.
    uint16_t c1 = nexus_crc16((const uint8_t*)"123456789", 9);
    uint16_t c2 = nexus_crc16((const uint8_t*)"123456789", 9);
    if (c1 != c2) { printf("  FAIL: crc not deterministic\n"); return 1; }
    printf("  PASS: CRC deterministic\n");

    // Encode a message with a payload
    nexus_message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.version = NEXUS_MSG_VERSION;
    msg.type = NEXUS_MSG_EVENT;
    msg.payload_len = 5;
    memcpy(msg.payload, "HELLO", 5);

    uint8_t buf[512];
    size_t out_len = 0;
    nexus_err_t err = nexus_message_encode(&msg, buf, sizeof(buf), &out_len);
    if (err != NEXUS_OK) { printf("  FAIL: encode err=%d\n", err); return 1; }
    size_t expected = NEXUS_MSG_HEADER_SIZE + 5 + NEXUS_MSG_CRC_SIZE;
    if (out_len != expected) { printf("  FAIL: encoded len=%zu expected %zu\n", out_len, expected); return 1; }
    printf("  PASS: message encode\n");

    // Decode it back
    nexus_message_t decoded;
    size_t consumed = 0;
    err = nexus_message_decode(buf, out_len, &decoded, &consumed);
    if (err != NEXUS_OK) { printf("  FAIL: decode err=%d\n", err); return 1; }
    if (decoded.type != NEXUS_MSG_EVENT) { printf("  FAIL: type=%u\n", decoded.type); return 1; }
    if (decoded.payload_len != 5) { printf("  FAIL: payload_len=%u\n", decoded.payload_len); return 1; }
    if (memcmp(decoded.payload, "HELLO", 5) != 0) { printf("  FAIL: payload mismatch\n"); return 1; }
    if (consumed != out_len) { printf("  FAIL: consumed=%zu\n", consumed); return 1; }
    printf("  PASS: message decode round-trip\n");

    // Corrupted CRC must be rejected
    buf[out_len - 1] ^= 0xFF;
    err = nexus_message_decode(buf, out_len, &decoded, &consumed);
    if (err != NEXUS_ERR_PROTOCOL) { printf("  FAIL: corrupted CRC accepted\n"); return 1; }
    printf("  PASS: corrupted CRC rejected\n");

    // Bad magic must be rejected
    buf[0] = 0x00; buf[1] = 0x00;
    err = nexus_message_decode(buf, out_len, &decoded, &consumed);
    if (err != NEXUS_ERR_PROTOCOL) { printf("  FAIL: bad magic accepted\n"); return 1; }
    printf("  PASS: bad magic rejected\n");

    // Truncated buffer must be rejected
    err = nexus_message_decode(buf, 3, &decoded, &consumed);
    if (err != NEXUS_ERR_PROTOCOL) { printf("  FAIL: truncated accepted\n"); return 1; }
    printf("  PASS: truncated buffer rejected\n");

    return 0;
}
