# Low Level Stream Protocol (C++)

순서가 보장되는 바이트 스트림(예: TCP, TLS 위의 스트림,
Unix 도메인 소켓, 파이프, 시리얼 링크 등) 위에서
바이너리 패킷을 인코딩/디코딩하기 위한 C++ 라이브러리입니다.

같은 프로토콜을 Java / C 구현과 공유하며,
헤더 전용(single-header) 버전도 함께 제공합니다.

## 프로토콜 개요

Java/C 구현과 동일한 레이아웃을 사용합니다.

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

- `include/streamprotocol/StreamProtocol.hpp`
  - 메인 C++ 클래스 정의.
- `include/streamprotocol/ParsedPacket.hpp`
  - 파싱 결과 DTO.
- `include/streamprotocol/PacketException.h`
  - 예외 계층 정의.
- `src/StreamProtocol.cpp`
  - 구현부.
- `StreamProtocol_single.hpp`
  - 위 헤더/구현을 하나로 합친 단일 헤더 버전.
- `examples/main.cpp`
  - 간단한 사용 예제.

## 기본 사용 예제

단일 헤더 버전을 사용하는 간단한 예제입니다.

```cpp
#include "StreamProtocol_single.hpp"

int main() {
    streamprotocol::StreamProtocol protocol;

    std::string message = "HelloPacket!";
    auto packet = protocol.toBytes(
        message,
        streamprotocol::StreamProtocol::UNFRAGED,
        42  // userField
    );

    auto parsed = protocol.parsePacket(packet);
    std::string extracted(parsed.Payload().begin(), parsed.Payload().end());
}
```

또는 `include/` + `src/` 를 함께 사용하는 일반적인 형태로도 사용할 수 있습니다.

```cpp
#include "streamprotocol/StreamProtocol.hpp"

int main() {
    streamprotocol::StreamProtocol protocol;
    // ...
}
```

## 빌드 예시

예제 프로그램을 간단히 빌드하려면 (GCC/Clang 기준):

```bash
cd cpp
g++ -std=c++17 -Iinclude examples/main.cpp src/StreamProtocol.cpp -o streamprotocol_example
./streamprotocol_example
```

실제 프로젝트에서는 `include/` 를 헤더 검색 경로에 추가하고,
`src/StreamProtocol.cpp` 를 함께 컴파일/링크하면 됩니다.
