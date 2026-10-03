#ifndef NEXUS_MESSAGE_H
#define NEXUS_MESSAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nexus_errors.h"

/*
 * Internal structured message protocol.
 *
 * Wire format (little-endian):
 *
 *   +--------+--------+------------+-------------+----------+---------+
 *   | magic  | ver    | type       | payload_len | payload  | crc16   |
 *   | 2 byte | 1 byte | 1 byte     | 2 bytes     | N bytes  | 2 bytes |
 *   +--------+--------+------------+-------------+----------+---------+
 *
 * A compact binary framing is chosen over JSON for the internal bus (MCU ->
 * capture/protocol/storage tasks) because it is deterministic, allocation-free
 * and cheap to parse. JSON is used at the *file* boundary (configs, tests,
 * device DB) where human editing matters more than wire efficiency. The PC
 * companion may carry JSON/ CBOR inside the payload; framing stays constant.
 */

#define NEXUS_MSG_MAGIC   0x584D  // "MX"
#define NEXUS_MSG_VERSION 1
#define NEXUS_MSG_MAX_PAYLOAD 256
#define NEXUS_MSG_HEADER_SIZE 6
#define NEXUS_MSG_CRC_SIZE 2

typedef enum {
    NEXUS_MSG_NONE = 0,
    NEXUS_MSG_EVENT,          // protocol/capture event for the timeline
    NEXUS_MSG_LOG,            // structured log record
    NEXUS_MSG_STATUS,         // system status update
    NEXUS_MSG_COMMAND,        // UI/service command
    NEXUS_MSG_RESPONSE,       // command response
    NEXUS_MSG_TEST_STEP,      // test engine step result
    NEXUS_MSG_CAPTURE,        // capture data block
    NEXUS_MSG_HEARTBEAT,
} nexus_msg_type_t;

typedef struct {
    uint8_t version;
    uint8_t type;
    uint16_t payload_len;
    uint8_t payload[NEXUS_MSG_MAX_PAYLOAD];
} nexus_message_t;

// CRC-16/CCITT-FALSE
uint16_t nexus_crc16(const uint8_t* data, size_t len);

nexus_err_t nexus_message_encode(const nexus_message_t* msg, uint8_t* out, size_t out_size, size_t* out_len);
nexus_err_t nexus_message_decode(const uint8_t* in, size_t in_len, nexus_message_t* out_msg, size_t* consumed);

#endif
