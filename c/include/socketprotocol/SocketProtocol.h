#pragma once

#include <stdint.h>
#include <stddef.h>

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

/**
 * 페이로드를 패킷으로 인코딩합니다. (동적 할당)
 *
 * @param payload         페이로드 바이트
 * @param payload_length  페이로드 길이
 * @param frag_flag       SP_FRAGED 또는 SP_UNFRAGED
 * @param payload_type    0~15 (4비트)
 * @param user_field      0~1023 (10비트)
 * @param out_packet      결과 패킷을 가리킬 포인터 (malloc으로 할당, 사용 후 free 필요)
 * @param out_length      결과 패킷 길이
 * @return                sp_result_t 코드
 */
sp_result_t sp_encode_packet(const uint8_t* payload,
                             size_t payload_length,
                             uint8_t frag_flag,
                             uint8_t payload_type,
                             uint16_t user_field,
                             uint8_t** out_packet,
                             size_t* out_length);

/**
 * 최대 패킷 크기 제한을 두고 인코딩합니다.
 *
 * @param max_packet_size 허용할 최대 패킷 크기 (바이트 단위)
 * 나머지 인자는 sp_encode_packet과 동일합니다.
 */
sp_result_t sp_encode_packet_limited(const uint8_t* payload,
                                     size_t payload_length,
                                     uint8_t frag_flag,
                                     uint8_t payload_type,
                                     uint16_t user_field,
                                     size_t max_packet_size,
                                     uint8_t** out_packet,
                                     size_t* out_length);

/**
 * 인코딩된 패킷을 파싱합니다.
 *
 * @param packet       패킷 바이트 배열 (헤더 + 페이로드 + CRC)
 * @param packet_len   패킷 길이
 * @param out_packet   결과를 채울 구조체 포인터
 *                     out_packet->payload 는 입력 packet 내부를 가리킵니다.
 * @return             sp_result_t 코드
 */
sp_result_t sp_parse_packet(const uint8_t* packet,
                            size_t packet_len,
                            sp_parsed_packet_t* out_packet);

#ifdef __cplusplus
}
#endif

