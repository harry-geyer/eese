#include <stdbool.h>

#include "cobs.h"

#include "uart_rings.h"
#include "crc32.h"
#include "system.h"


#define ITF_PACKET_BUF_SIZE                 128
#define ITF_PACKET_VERSION                  1


typedef enum {
    ITF_PACKET_OUT_TYPE_NOP = 1,
    ITF_PACKET_OUT_TYPE_MEASUREMENTS = 2,
    ITF_PACKET_OUT_TYPE_HEALTH = 3,
    ITF_PACKET_OUT_TYPE_EVENT = 4,
} _itf_packet_out_type_t;


typedef enum {
    ITF_PACKET_IN_TYPE_NOP = 1,
    ITF_PACKET_IN_TYPE_RESET = 2,
} _itf_packet_in_type_t;


typedef struct {
    uint8_t version;
    uint8_t type; /* _itf_packet_out_type_t / _itf_packet_in_type_t */
} __attribute__((packed)) _itf_packet_header_t;


static bool _itf_send_packet(_itf_packet_out_type_t type, uint8_t* payload, uint32_t len);
static uint32_t _itf_process_packet(uint8_t* buf, uint32_t len);


static uint8_t _itf_packet_buf[ITF_PACKET_BUF_SIZE] = {0};


bool itf_send_nop(void)
{
    return _itf_send_packet(ITF_PACKET_OUT_TYPE_NOP, NULL, 0);
}


void itf_iterate(void)
{
    uint32_t len = 1;
    while (len) {
        len = uart_rings_in_drain(_itf_packet_buf, ITF_PACKET_BUF_SIZE);
        len -= _itf_process_packet(_itf_packet_buf, len);
    }
}


static bool _itf_send_packet(_itf_packet_out_type_t type, uint8_t* payload, uint32_t len)
{
    if (ITF_PACKET_BUF_SIZE <= len) {
        /* <= not < as using COBS, final packet will always be 1 byte
         * longer than payload */
        return false;
    }
    cobs_enc_ctx_t cobs_ctx = {0};
    _itf_packet_buf[0] = COBS_TINYFRAME_SENTINEL_VALUE;
    _itf_packet_buf[ITF_PACKET_BUF_SIZE-1] = COBS_TINYFRAME_SENTINEL_VALUE;
    if (COBS_RET_SUCCESS != cobs_encode_inc_begin(_itf_packet_buf, ITF_PACKET_BUF_SIZE, &cobs_ctx)) {
        return false;
    }
    _itf_packet_header_t header;
    header.version = ITF_PACKET_VERSION;
    header.type = type;
    if (COBS_RET_SUCCESS != cobs_encode_inc(&cobs_ctx, &header, sizeof(_itf_packet_header_t))) {
        return false;
    }
    if (COBS_RET_SUCCESS != cobs_encode_inc(&cobs_ctx, payload, len)) {
        return false;
    }
    uint32_t crc = crc32((uint8_t*)&header, sizeof(_itf_packet_header_t), CRC32_DEFAULT_START);
    crc = crc32(payload, len, crc);
    if (COBS_RET_SUCCESS != cobs_encode_inc(&cobs_ctx, &crc, sizeof(uint32_t))) {
        return false;
    }
    size_t packet_len = 0;
    if (COBS_RET_SUCCESS != cobs_encode_inc_end(&cobs_ctx, &packet_len)) {
        return false;
    }
    return packet_len == uart_rings_out_add((uint8_t*)_itf_packet_buf, packet_len);
}


static uint32_t _itf_process_packet(uint8_t* buf, uint32_t len)
{
    static uint8_t packet[ITF_PACKET_BUF_SIZE];
    cobs_decode_inc_ctx_t cobs_ctx = {0};
    cobs_decode_inc_args_t cobs_args = {0};
    cobs_args.dec_dst = packet;
    cobs_args.dec_dst_max = ITF_PACKET_BUF_SIZE;
    cobs_args.enc_src = buf;
    cobs_args.enc_src_max = len;
    if (COBS_RET_SUCCESS != cobs_decode_inc_begin(&cobs_ctx)) {
        /* unable init cobs, probably bad pointer? */
        return len;
    }
    size_t out_enc_src_len = 0;
    size_t out_dec_dst_len = 0;
    bool out_decode_complete = false;
    if (COBS_RET_SUCCESS != cobs_decode_inc(&cobs_ctx, &cobs_args, &out_enc_src_len, &out_dec_dst_len, &out_decode_complete)) {
        /* fail to decode, toss packet */
        return len;
    }
    if (!out_decode_complete) {
        /* not complete, for whatever reason, toss packet */
        return len;
    }
    if (sizeof(_itf_packet_header_t) > out_dec_dst_len) {
        /* packet too small, assume broken */
        return len;
    }
    if (crc32(packet, out_dec_dst_len, CRC32_DEFAULT_START)) {
        /* CRC32 of whole packet (including embedded CRC) will be 0 if
         * correct, if incorrect, throw away packet */
        return out_enc_src_len;
    }
    _itf_packet_header_t* header = (_itf_packet_header_t*)packet;
    if (ITF_PACKET_VERSION != header->version) {
        /* wrong packet version */
        return out_enc_src_len;
    }
    switch (header->type) {
        case ITF_PACKET_IN_TYPE_NOP:
            break;
        case ITF_PACKET_IN_TYPE_RESET:
            system_reset();
            break;
        default:
            /* Unknown packet type */
            break;
    }
    return out_enc_src_len;
}
