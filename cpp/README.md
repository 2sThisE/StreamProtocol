# Low Level Socket Protocol (C++)

로우레벨 TCP 패킷 인코딩/디코딩을 위한 C++ 라이브러리입니다.  
이 디렉터리는 C++ 구현을 담고 있으며, **싱글 헤더 사용**을 기본으로 권장합니다.

## 사용 권장 방식: 단일 헤더

가장 간단한 사용 방법은 루트에 있는 `SocketProtocol_single.hpp` 파일 **하나만** 프로젝트에 복사해서 쓰는 것입니다.

### 1. 헤더 포함

```cpp
#include "SocketProtocol_single.hpp"

int main() {
    socketprotocol::SocketProtocol protocol;

    std::string message = "HelloPacket!";
    auto packet = protocol.toBytes(message,
                                   socketprotocol::SocketProtocol::UNFRAGED,
                                   42); // userField

    auto parsed = protocol.parsePacket(packet);
    std::string extracted(parsed.Payload().begin(), parsed.Payload().end());
}
```

### 2. 패킷 구조

Java 구현과 동일한 포맷을 사용합니다.

```text
[ 8바이트 헤더 ][ 페이로드 ][ 4바이트 CRC32 ]
```

- 헤더는 64비트 `uint64_t` 값으로 구성되며, little-endian으로 직렬화됩니다.
- 비트 레이아웃:
  - bits 0–3  : `protocolVersion` (4비트)
  - bits 4–48 : `packetLength` (45비트, 헤더 + 페이로드 + CRC 총 길이)
  - bit  49   : `fragmentFlag` (1비트)
  - bits 50–53: `payloadType` (4비트)
  - bits 54–63: `userField` (10비트)

CRC32는 폴리노미얼 `0xEDB88320`를 사용하는 일반적인 CRC-32 구현입니다.

### 3. API 개요 (싱글 헤더)

`SocketProtocol_single.hpp` 안에는 다음 클래스들이 정의되어 있습니다.

- `socketprotocol::PacketException` 및 파생 예외
  - `PayloadTooLargeException`
  - `BufferTooSmallException`
  - `PacketSizeMismatch`
  - `InvalidCRCException`
- `socketprotocol::ParsedPacket`
  - 헤더 필드와 페이로드를 조회하는 getter 제공
- `socketprotocol::SocketProtocol`
  - `std::vector<uint8_t> toBytes(const std::string& payload, uint8_t fragFlag = UNFRAGED, uint16_t userValue = 0);`
  - `std::vector<uint8_t> toBytes(const std::vector<uint8_t>& payload, uint8_t fragFlag = UNFRAGED, uint16_t userValue = 0);`
  - `ParsedPacket parsePacket(const std::vector<uint8_t>& packetBytes);`

각 메서드는 다음과 같은 방어 로직을 포함합니다.

- `payload == nullptr` → `std::invalid_argument`
- `fragFlag`가 `FRAGED` / `UNFRAGED`가 아닌 경우 → `std::invalid_argument`
- `payloadType`이 0~15 범위를 벗어난 경우 → `std::invalid_argument`
- `userField`가 0~1023 범위를 벗어난 경우 → `std::invalid_argument`
- 패킷 총 길이가 헤더가 표현 가능한 최대값을 넘어가는 경우 → `PayloadTooLargeException`
- 수신 패킷이 헤더+CRC 최소 길이보다 짧은 경우 → `BufferTooSmallException`
- 헤더의 길이 필드와 실제 버퍼 길이가 다른 경우 → `PacketSizeMismatch`
- CRC 검증 실패 → `InvalidCRCException`

## 대안: include/src 구조 사용

보다 전통적인 라이브러리 구조로 사용하고 싶다면, 다음 폴더들을 포함해서 프로젝트에 추가할 수 있습니다.

- `include/socketprotocol/PacketException.h`
- `include/socketprotocol/ParsedPacket.hpp`
- `include/socketprotocol/SocketProtocol.hpp`
- `src/SocketProtocol.cpp`

이 경우에는 다음과 같이 포함하여 사용합니다.

```cpp
#include "socketprotocol/SocketProtocol.hpp"

int main() {
    socketprotocol::SocketProtocol protocol;
    // ...
}
```

싱글 헤더 버전과 동일한 비트 레이아웃과 예외 처리 규칙을 따릅니다.

