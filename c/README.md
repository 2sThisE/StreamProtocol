# Low Level Stream Protocol (C)

순서가 보장되는 바이트 스트림(예: TCP, TLS 위의 스트림,
Unix 도메인 소켓, 파이프, 시리얼 링크 등) 위에서
바이너리 패킷을 인코딩/디코딩하기 위한 C 구현입니다.

Java / C++ 구현과 동일한 바이너리 포맷을 사용하므로,
여러 언어 간에 같은 패킷을 주고받을 수 있습니다.

## 프로토콜 개요

```text
[ 8바이트 헤더 ][ 페이로드 ][ 4바이트 CRC32 ]
```

- 헤더는 64비트 `uint64_t` 값을 little-endian 으로 표현한 것입니다.
- 헤더 비트 배열:
  - 비트 0–3  : `protocolVersion` (4비트)
  - 비트 4–48 : `packetLength` (45비트, 헤더 + 페이로드 + CRC 전체 길이)
  - 비트 49   : `fragmentFlag` (1비트)
  - 비트 50–53: `payloadType` (4비트)
  - 비트 54–63: `userField` (10비트)
- CRC32 는 다항식 `0xEDB88320` 을 사용합니다.

## 구성

- `include/streamprotocol/StreamProtocol.h`
  - C API 선언.
- `src/StreamProtocol.c`
  - 구현부.
- `StreamProtocol_single.h`
  - 선언과 구현을 하나로 합친 단일 헤더 버전.
- `examples/main.c`
  - 간단한 사용 예제.

## C API 개요

헤더 인클루드는 다음과 같습니다.

```c
#include "streamprotocol/StreamProtocol.h"
```

주요 상수:

- `SP_HEADER_SIZE` : 헤더 길이 (8바이트)
- `SP_FRAGED` / `SP_UNFRAGED` : fragment flag 값

결과 코드(`sp_result_t`):

- `SP_OK`
- `SP_ERR_BUFFER_TOO_SMALL`
- `SP_ERR_PAYLOAD_TOO_LARGE`
- `SP_ERR_INVALID_ARGUMENT`
- `SP_ERR_LENGTH_MISMATCH`
- `SP_ERR_CRC_MISMATCH`

파싱 결과 구조체(`sp_parsed_packet_t`):

```c
typedef struct sp_parsed_packet_s {
    uint8_t  protocol_version;
    uint64_t packet_length;    /* 헤더 + 페이로드 + CRC */
    uint8_t  fragment_flag;
    uint8_t  payload_type;
    uint16_t user_field;
    const uint8_t* payload;    /* 원본 패킷 버퍼 내부 포인터 */
    size_t   payload_length;
} sp_parsed_packet_t;
```

인코딩 함수:

```c
sp_result_t sp_encode_packet(const uint8_t* payload,
                             size_t payload_length,
                             uint8_t frag_flag,
                             uint8_t payload_type,
                             uint16_t user_field,
                             uint8_t** out_packet,
                             size_t* out_length);
```

- `out_packet` 은 `malloc` 으로 할당되며, 사용 후 호출자가 `free()` 해야 합니다.
- `sp_encode_packet_limited` 를 사용하면 최대 패킷 크기를 제한할 수 있습니다.

디코딩 함수:

```c
sp_result_t sp_parse_packet(const uint8_t* packet,
                            size_t packet_len,
                            sp_parsed_packet_t* out_packet);
```

- `out_packet->payload` 는 입력 `packet` 버퍼 내부를 가리키므로,
  `packet` 이 유효한 동안에만 사용해야 합니다.

## 간단 예제

`examples/main.c` 를 참고하면 전체 흐름을 볼 수 있습니다.

```c
const char* msg = "Hello, C StreamProtocol!";
uint8_t* packet = NULL;
size_t packet_len = 0;

sp_result_t res = sp_encode_packet(
    (const uint8_t*)msg,
    strlen(msg),
    SP_UNFRAGED,
    0x01u,
    42u,
    &packet,
    &packet_len
);

sp_parsed_packet_t parsed;
res = sp_parse_packet(packet, packet_len, &parsed);
```

이 패킷은 Java/C++ 구현에서 그대로 파싱할 수 있습니다.
