# Low Level Socket Protocol (C)

이 디렉터리는 C 언어용 패킷 인코딩/디코딩 구현입니다.  
Java/C++ 버전과 동일한 8바이트 헤더 + CRC32 구조를 사용합니다.

## 패킷 구조

```text
[ 8바이트 헤더 ][ 페이로드 ][ 4바이트 CRC32 ]
```

- 헤더는 64비트 `uint64_t` 값으로 인코딩되며, little-endian 바이트 순서를 사용합니다.
- 비트 레이아웃:
  - 비트 0–3  : `protocolVersion` (4비트)
  - 비트 4–48 : `packetLength` (45비트, 헤더 + 페이로드 + CRC 전체 길이)
  - 비트 49   : `fragmentFlag` (1비트)
  - 비트 50–53: `payloadType` (4비트)
  - 비트 54–63: `userField` (10비트)

CRC32는 폴리노미얼 `0xEDB88320`를 사용하는 표준 CRC-32 구현입니다.

## 파일 구성

- `include/socketprotocol/SocketProtocol.h`
  - 퍼블릭 C API 헤더
- `src/SocketProtocol.c`
  - 구현 파일
- `examples/main.c`
  - 간단한 사용 예제 (인코딩 → 디코딩)


## C API

헤더: `#include "socketprotocol/SocketProtocol.h"`

### 상수

- `SP_HEADER_SIZE` : 헤더 크기(8바이트)
- `SP_FRAGED` / `SP_UNFRAGED` : 단편화 플래그 값

### 에러 코드 (`sp_result_t`)

- `SP_OK` : 성공
- `SP_ERR_BUFFER_TOO_SMALL` : 버퍼가 최소 헤더+CRC 길이보다 작음
- `SP_ERR_PAYLOAD_TOO_LARGE` : 헤더/제한을 초과하는 패킷 크기
- `SP_ERR_INVALID_ARGUMENT` : 인자 범위 오류 (플래그, 타입, userField 등)
- `SP_ERR_LENGTH_MISMATCH` : 헤더의 길이 필드와 실제 버퍼 길이 불일치
- `SP_ERR_CRC_MISMATCH` : CRC32 검증 실패

### 파싱 결과 구조체

```c
typedef struct sp_parsed_packet_s {
    uint8_t  protocol_version;
    uint64_t packet_length;    /* 헤더 + 페이로드 + CRC */
    uint8_t  fragment_flag;
    uint8_t  payload_type;
    uint16_t user_field;
    const uint8_t* payload;    /* 입력 패킷 버퍼 내부를 가리킴 */
    size_t   payload_length;
} sp_parsed_packet_t;
```

### 인코딩 함수

```c
sp_result_t sp_encode_packet(const uint8_t* payload,
                             size_t payload_length,
                             uint8_t frag_flag,
                             uint8_t payload_type,
                             uint16_t user_field,
                             uint8_t** out_packet,
                             size_t* out_length);
```

- `out_packet` 는 `malloc`으로 할당되며, 사용 후 `free()` 해야 합니다.
- `sp_encode_packet_limited` 를 사용하면 추가로 `max_packet_size` 제한을 둘 수 있습니다.

### 디코딩 함수

```c
sp_result_t sp_parse_packet(const uint8_t* packet,
                            size_t packet_len,
                            sp_parsed_packet_t* out_packet);
```

- `out_packet->payload` 는 입력 `packet` 버퍼 내부를 가리키므로,  
  `packet`의 생명 주기 동안만 유효합니다.

## 간단한 예제

`examples/main.c` 참고:

```c
const char* msg = "Hello, C SocketProtocol!";
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

이렇게 하면 Java / C++ 구현과 동일한 형식의 패킷을 C에서 생성/파싱할 수 있습니다.
