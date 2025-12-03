#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 헤더/플래그 상수 */
#define SP_HEADER_SIZE 8U
#define SP_FRAGED      0x01U
#define SP_UNFRAGED    0x00U

/* 에러 코드 */
typedef enum sp_result_e {
    SP_OK = 0,
    SP_ERR_BUFFER_TOO_SMALL,
    SP_ERR_PAYLOAD_TOO_LARGE,
    SP_ERR_INVALID_ARGUMENT,
    SP_ERR_LENGTH_MISMATCH,
    SP_ERR_CRC_MISMATCH
} sp_result_t;

/* 파싱된 패킷 정보 */
typedef struct sp_parsed_packet_s {
    uint8_t  protocol_version;
    uint64_t packet_length;     /* 헤더 + 페이로드 + CRC */
    uint8_t  fragment_flag;
    uint8_t  payload_type;
    uint16_t user_field;
    const uint8_t* payload;     /* 페이로드 시작 포인터 (입력 버퍼 내부를 가리킴) */
    size_t   payload_length;    /* 페이로드 길이 */
} sp_parsed_packet_t;

/* 45비트 길이 필드의 최대 값 */
#define SP_MAX_HEADER_LENGTH_VALUE 0x1FFFFFFFFFFFull

/* 내부 CRC32 구현 (Java/C++ 버전과 동일 폴리노미얼) */
static inline uint32_t sp_crc32_inline(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

/* 공통 인코딩 내부 함수 (inline) */
static inline sp_result_t sp_encode_internal_inline(const uint8_t* payload,
                                                    size_t payload_length,
                                                    uint8_t frag_flag,
                                                    uint8_t payload_type,
                                                    uint16_t user_field,
                                                    uint64_t max_packet_size,
                                                    uint8_t** out_packet,
                                                    size_t* out_length) {
    if (!payload || !out_packet || !out_length) {
        return SP_ERR_INVALID_ARGUMENT;
    }

    /* fragment flag 검증 */
    if (frag_flag != SP_FRAGED && frag_flag != SP_UNFRAGED) {
        return SP_ERR_INVALID_ARGUMENT;
    }

    /* payload type 검증 (4비트) */
    if (payload_type > 0x0F) {
        return SP_ERR_INVALID_ARGUMENT;
    }

    /* user field 검증 (10비트) */
    if (user_field > 0x3FFu) {
        return SP_ERR_INVALID_ARGUMENT;
    }

    /* 패킷 전체 길이 계산 */
    uint64_t total_len_64 = SP_HEADER_SIZE + (uint64_t)payload_length + 4u;

    /* 헤더 비트폭 및 호출자가 지정한 max_packet_size를 기준으로 검증 */
    uint64_t header_limit = SP_MAX_HEADER_LENGTH_VALUE;
    uint64_t effective_max = (max_packet_size > 0 && max_packet_size < header_limit)
                                 ? max_packet_size
                                 : header_limit;

    if (total_len_64 > effective_max) {
        return SP_ERR_PAYLOAD_TOO_LARGE;
    }

    size_t total_len = (size_t)total_len_64;

    uint8_t* buf = (uint8_t*)malloc(total_len);
    if (!buf) {
        return SP_ERR_PAYLOAD_TOO_LARGE;
    }

    /* 64비트 헤더 구성 (little-endian) */
    uint8_t protocol_version = 1u;
    uint64_t header_value = 0;
    header_value |= ((uint64_t)protocol_version & 0x0Fu) << 0;
    header_value |= (total_len_64 & 0x1FFFFFFFFFFFull) << 4;
    header_value |= ((uint64_t)frag_flag & 0x01u) << 49;
    header_value |= ((uint64_t)payload_type & 0x0Fu) << 50;
    header_value |= ((uint64_t)user_field & 0x3FFu) << 54;

    for (size_t i = 0; i < SP_HEADER_SIZE; ++i) {
        buf[i] = (uint8_t)((header_value >> (i * 8)) & 0xFFu);
    }

    /* 페이로드 복사 */
    if (payload_length > 0) {
        for (size_t i = 0; i < payload_length; ++i) {
            buf[SP_HEADER_SIZE + i] = payload[i];
        }
    }

    /* CRC 계산 (헤더 + 페이로드) */
    uint32_t crc = sp_crc32_inline(buf, SP_HEADER_SIZE + payload_length);
    size_t crc_offset = total_len - 4u;
    buf[crc_offset + 0] = (uint8_t)((crc >> 0) & 0xFFu);
    buf[crc_offset + 1] = (uint8_t)((crc >> 8) & 0xFFu);
    buf[crc_offset + 2] = (uint8_t)((crc >> 16) & 0xFFu);
    buf[crc_offset + 3] = (uint8_t)((crc >> 24) & 0xFFu);

    *out_packet = buf;
    *out_length = total_len;
    return SP_OK;
}

static inline sp_result_t sp_encode_packet(const uint8_t* payload,
                                           size_t payload_length,
                                           uint8_t frag_flag,
                                           uint8_t payload_type,
                                           uint16_t user_field,
                                           uint8_t** out_packet,
                                           size_t* out_length) {
    return sp_encode_internal_inline(payload, payload_length, frag_flag, payload_type, user_field,
                                     0, out_packet, out_length);
}

static inline sp_result_t sp_encode_packet_limited(const uint8_t* payload,
                                                   size_t payload_length,
                                                   uint8_t frag_flag,
                                                   uint8_t payload_type,
                                                   uint16_t user_field,
                                                   size_t max_packet_size,
                                                   uint8_t** out_packet,
                                                   size_t* out_length) {
    return sp_encode_internal_inline(payload, payload_length, frag_flag, payload_type, user_field,
                                     (uint64_t)max_packet_size, out_packet, out_length);
}

static inline sp_result_t sp_parse_packet(const uint8_t* packet,
                                          size_t packet_len,
                                          sp_parsed_packet_t* out_packet) {
    if (!packet || !out_packet) {
        return SP_ERR_INVALID_ARGUMENT;
    }

    if (packet_len < SP_HEADER_SIZE + 4u) {
        return SP_ERR_BUFFER_TOO_SMALL;
    }

    /* 64비트 헤더 읽기 (little-endian) */
    uint64_t header_value = 0;
    for (size_t i = 0; i < SP_HEADER_SIZE; ++i) {
        header_value |= ((uint64_t)packet[i]) << (i * 8);
    }

    uint8_t proto_version = (uint8_t)((header_value >> 0) & 0x0Fu);
    uint64_t packet_length64 = (header_value >> 4) & 0x1FFFFFFFFFFFull;
    uint8_t fragment_flag = (uint8_t)((header_value >> 49) & 0x01u);
    uint8_t payload_type = (uint8_t)((header_value >> 50) & 0x0Fu);
    uint16_t user_field = (uint16_t)((header_value >> 54) & 0x3FFu);

    if (packet_length64 < SP_HEADER_SIZE + 4u) {
        return SP_ERR_BUFFER_TOO_SMALL;
    }
    if (packet_length64 > SP_MAX_HEADER_LENGTH_VALUE) {
        return SP_ERR_PAYLOAD_TOO_LARGE;
    }

    size_t packet_length = (size_t)packet_length64;

    if (packet_len != packet_length) {
        return SP_ERR_LENGTH_MISMATCH;
    }

    /* CRC 추출 (마지막 4바이트, little-endian) */
    size_t crc_offset = packet_length - 4u;
    uint32_t received_crc = 0;
    received_crc |= ((uint32_t)packet[crc_offset + 0]) << 0;
    received_crc |= ((uint32_t)packet[crc_offset + 1]) << 8;
    received_crc |= ((uint32_t)packet[crc_offset + 2]) << 16;
    received_crc |= ((uint32_t)packet[crc_offset + 3]) << 24;

    /* 헤더 + 페이로드에 대한 CRC 계산 */
    uint32_t computed_crc = sp_crc32_inline(packet, packet_length - 4u);
    if (computed_crc != received_crc) {
        return SP_ERR_CRC_MISMATCH;
    }

    /* 결과 구조체 채우기 (payload는 입력 버퍼 내부를 가리킴) */
    out_packet->protocol_version = proto_version;
    out_packet->packet_length = packet_length64;
    out_packet->fragment_flag = fragment_flag;
    out_packet->payload_type = payload_type;
    out_packet->user_field = user_field;
    out_packet->payload = packet + SP_HEADER_SIZE;
    out_packet->payload_length = packet_length - SP_HEADER_SIZE - 4u;

    return SP_OK;
}

#ifdef __cplusplus
}
#endif

