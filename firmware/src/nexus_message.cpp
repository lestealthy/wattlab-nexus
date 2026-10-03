#include "nexus_message.h"
#include <string.h>

uint16_t nexus_crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; b++) {
            if (crc & 0x8000) crc = (uint16_t)((crc << 1) ^ 0x1021);
            else crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

nexus_err_t nexus_message_encode(const nexus_message_t* msg, uint8_t* out, size_t out_size, size_t* out_len) {
    if (!msg || !out || !out_len) return NEXUS_ERR_INVALID_PARAM;
    if (msg->payload_len > NEXUS_MSG_MAX_PAYLOAD) return NEXUS_ERR_INVALID_PARAM;

    size_t total = NEXUS_MSG_HEADER_SIZE + msg->payload_len + NEXUS_MSG_CRC_SIZE;
    if (out_size < total) return NEXUS_ERR_BUFFER_OVERFLOW;

    size_t i = 0;
    out[i++] = (uint8_t)(NEXUS_MSG_MAGIC & 0xFF);
    out[i++] = (uint8_t)((NEXUS_MSG_MAGIC >> 8) & 0xFF);
    out[i++] = msg->version;
    out[i++] = msg->type;
    out[i++] = (uint8_t)(msg->payload_len & 0xFF);
    out[i++] = (uint8_t)((msg->payload_len >> 8) & 0xFF);
    if (msg->payload_len) {
        memcpy(&out[i], msg->payload, msg->payload_len);
        i += msg->payload_len;
    }

    uint16_t crc = nexus_crc16(out, i);
    out[i++] = (uint8_t)(crc & 0xFF);
    out[i++] = (uint8_t)((crc >> 8) & 0xFF);

    *out_len = i;
    return NEXUS_OK;
}

nexus_err_t nexus_message_decode(const uint8_t* in, size_t in_len, nexus_message_t* out_msg, size_t* consumed) {
    if (!in || !out_msg || !consumed) return NEXUS_ERR_INVALID_PARAM;
    *consumed = 0;

    if (in_len < NEXUS_MSG_HEADER_SIZE + NEXUS_MSG_CRC_SIZE) return NEXUS_ERR_PROTOCOL;

    uint16_t magic = (uint16_t)(in[0] | (in[1] << 8));
    if (magic != NEXUS_MSG_MAGIC) return NEXUS_ERR_PROTOCOL;

    uint8_t version = in[2];
    uint8_t type = in[3];
    uint16_t payload_len = (uint16_t)(in[4] | (in[5] << 8));

    if (payload_len > NEXUS_MSG_MAX_PAYLOAD) return NEXUS_ERR_PROTOCOL;

    size_t total = NEXUS_MSG_HEADER_SIZE + payload_len + NEXUS_MSG_CRC_SIZE;
    if (in_len < total) return NEXUS_ERR_PROTOCOL;

    uint16_t crc_calc = nexus_crc16(in, NEXUS_MSG_HEADER_SIZE + payload_len);
    uint16_t crc_wire = (uint16_t)(in[NEXUS_MSG_HEADER_SIZE + payload_len] |
                                   (in[NEXUS_MSG_HEADER_SIZE + payload_len + 1] << 8));
    if (crc_calc != crc_wire) return NEXUS_ERR_PROTOCOL;

    memset(out_msg, 0, sizeof(*out_msg));
    out_msg->version = version;
    out_msg->type = type;
    out_msg->payload_len = payload_len;
    if (payload_len) memcpy(out_msg->payload, &in[NEXUS_MSG_HEADER_SIZE], payload_len);

    *consumed = total;
    return NEXUS_OK;
}
