# StreamProtocol Monorepo

StreamProtocol은 순서가 보장되는 바이트 스트림(예: TCP, TLS 위의 스트림,
Unix 도메인 소켓, 파이프, 시리얼 링크 등) 위에서 사용할 수 있는
경량 바이너리 패킷 프레이밍 프로토콜입니다.

이 저장소는 같은 프로토콜을 Java / C++ / C 세 언어로 구현한 모노레포입니다.

## 공통 프로토콜 특성

- 8바이트 고정 헤더(64비트, little-endian)
- 가변 길이 페이로드
- 4바이트 CRC32 (다항식 `0xEDB88320`)

헤더 비트 배열:

- 비트 0–3  : `protocolVersion` (4비트)
- 비트 4–48 : `packetLength` (45비트, 헤더 + 페이로드 + CRC 전체 길이)
- 비트 49   : `fragmentFlag` (1비트)
- 비트 50–53: `payloadType` (4비트)
- 비트 54–63: `userField` (10비트)

이 규칙만 지키면, 어떤 “순서 보장 바이트 스트림” 위에서도 동일한 패킷 포맷을 사용할 수 있습니다.

## 서브디렉터리 구조

- `java/`  
  Maven 기반 Java 라이브러리
  - `src/main/java/streamprotocol/StreamProtocol.java`
  - `src/main/java/streamprotocol/ParsedPacket.java`
  - `src/main/java/streamprotocol/PacketException` 계열 예외
  - `src/test/java/streamprotocol/StreamProtocolSample.java` 샘플

- `cpp/`  
  C++ 라이브러리 및 단일 헤더 버전
  - `include/streamprotocol/StreamProtocol.hpp` + `src/StreamProtocol.cpp`
  - `StreamProtocol_single.hpp` (단일 헤더로 사용 가능한 버전)
  - `examples/main.cpp` 샘플

- `c/`  
  C 라이브러리 및 단일 헤더 버전
  - `include/streamprotocol/StreamProtocol.h` + `src/StreamProtocol.c`
  - `StreamProtocol_single.h` (단일 헤더로 사용 가능한 버전)
  - `examples/main.c` 샘플

각 언어별 디렉터리의 README에서, 해당 언어의 사용 예제와 빌드/실행 방법을 확인할 수 있습니다.
