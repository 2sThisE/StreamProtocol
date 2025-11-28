# SocketProtocol Monorepo

이 저장소는 동일한 바이너리 패킷 포맷을 사용하는 SocketProtocol 구현을
여러 언어(Java / C++ / C)로 모아둔 모노레포입니다.

공통 프로토콜:

- 8바이트 헤더(64비트, little-endian)
- 가변 길이 페이로드
- 4바이트 CRC32 (폴리노미얼 `0xEDB88320`)

헤더 비트 레이아웃:

- 비트 0–3  : `protocolVersion` (4비트)
- 비트 4–48 : `packetLength` (45비트, 헤더 + 페이로드 + CRC 전체 길이)
- 비트 49   : `fragmentFlag` (1비트)
- 비트 50–53: `payloadType` (4비트)
- 비트 54–63: `userField` (10비트)

## 디렉터리 구조

- `java/`  
  Maven 기반 Java 라이브러리
  - `src/main/java/socketprotocol/SocketProtocol.java`
  - `src/main/java/socketprotocol/ParsedPacket.java`
  - `src/main/java/socketprotocol/PacketException` 계열 예외
  - `src/test/java/socketprotocol/SocketProtocolSample.java` 샘플

- `cpp/`  
  C++ 라이브러리 및 싱글 헤더 버전
  - `include/socketprotocol/SocketProtocol.hpp` + `src/SocketProtocol.cpp`
  - `SocketProtocol_single.hpp` (헤더 하나로 사용하는 버전)
  - `examples/main.cpp` 샘플

- `c/`  
  C 라이브러리 및 싱글 헤더 버전
  - `include/socketprotocol/SocketProtocol.h` + `src/SocketProtocol.c`
  - `SocketProtocol_single.h` (헤더 하나로 사용하는 버전)
  - `examples/main.c` 샘플

각 언어별 디렉터리 안의 README를 참고하면,
해당 언어에서의 상세 사용법과 빌드 예시를 확인할 수 있습니다.

