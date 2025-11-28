# Low Level Socket Protocol (Java)

Java용 로우레벨 패킷 인코딩/디코딩 라이브러리입니다.  
단일 TCP 스트림 위에서 사용할 수 있는 간단한 바이너리 패킷 포맷을 제공합니다.

## 패킷 구조

전체 패킷 레이아웃은 다음과 같습니다.

```text
[ 8바이트 헤더 ][ 페이로드 ][ 4바이트 CRC32 ]
```

- 헤더와 페이로드 전체에 대해 CRC32를 계산하고, 마지막 4바이트에 기록합니다.
- 엔디언: **little-endian**

### 64비트 헤더 비트 레이아웃

헤더는 64비트 `long` 하나로 구성되며, 각 비트의 의미는 다음과 같습니다.

- 비트 0–3  : `protocolVersion` (4비트)
- 비트 4–48 : `packetLength` (45비트, 헤더 + 페이로드 + CRC 전체 길이)
- 비트 49   : `fragmentFlag` (1비트, 단편화 여부)
- 비트 50–53: `payloadType` (4비트, 사용자 정의 타입)
- 비트 54–63: `userField` (10비트, 사용자 메타데이터)

패킷 길이 필드는 45비트를 사용하므로, 이론적으로 매우 큰 패킷까지 표현할 수 있지만  
Java 배열 한계를 고려해 구현에서는 다음 상수를 제공합니다.

- `SocketProtocol.MAX_PACKET_LENGTH`  
  헤더 + 페이로드 + CRC 를 모두 포함한 최대 패킷 길이
- `SocketProtocol.MAX_PAYLOAD_LENGTH`  
  페이로드만의 최대 길이 (헤더/CRC 제외)

## 주요 클래스

- `socketprotocol.SocketProtocol`
  - 패킷 인코딩(`toBytes`) 및 디코딩(`parsePacket`)을 담당하는 핵심 클래스
- `socketprotocol.ParsedPacket`
  - `parsePacket` 결과를 담는 불변 객체
  - 헤더 필드 및 페이로드를 조회할 수 있는 getter 제공
- 예외 클래스
  - `PacketException` : 모든 패킷 관련 예외의 기본 클래스
  - `BufferTooSmallException` : 버퍼가 최소 헤더+CRC 길이보다 작은 경우
  - `PayloadTooLargeException` : 패킷 길이가 허용 가능한 최대 크기를 초과한 경우
  - `PacketSizeMismatch` : 헤더의 길이 필드와 실제 버퍼 길이가 다른 경우
  - `InvalidCRCException` : CRC 검증 실패

## 의존성 / 빌드

이 모듈은 Maven 프로젝트입니다.

```bash
cd Low_Level_socket_protocol_Java
mvn test
```

위 명령으로 컴파일 및 테스트를 실행할 수 있습니다.

## 기본 사용법

### 1. 인코딩 (toBytes)

```java
SocketProtocol protocol = new SocketProtocol();

byte[] payload = "Hello, TcpHelper!".getBytes();
byte fragFlag = SocketProtocol.UNFRAGED; // 또는 FRAGED
byte payloadType = 0x01;                 // 0~15 (4비트)
int userField = 42;                      // 0~1023 (10비트)

// 최대 크기 제한 없이 인코딩 (내부적으로 MAX_PACKET_LENGTH를 기준으로 검증)
byte[] packet = protocol.toBytes(payload, fragFlag, payloadType, userField);
```

추가로, 호출 시점에 상한 버퍼 크기를 직접 지정할 수도 있습니다.

```java
int bufferSize = 1024; // 이 호출에서 허용할 최대 패킷 크기
byte[] packet = protocol.toBytes(payload, fragFlag, payloadType, userField, bufferSize);
```

예외 발생 조건:

- `payload == null` → `IllegalArgumentException`
- `fragFlag` 가 `FRAGED`/`UNFRAGED`가 아닌 경우 → `IllegalArgumentException`
- `payloadType` 이 0~15 범위를 벗어난 경우 → `IllegalArgumentException`
- `userField` 가 0~1023 범위를 벗어난 경우 → `IllegalArgumentException`
- 계산된 패킷 길이가 `MAX_PACKET_LENGTH`를 넘어가는 경우 → `PayloadTooLargeException`
- 버퍼 상한 버전(`toBytes(..., bufferSize)`)에서
  - `bufferSize < HEADER_SIZE + 4` → `BufferTooSmallException`
  - 계산된 패킷 길이가 `bufferSize` 초과 → `PayloadTooLargeException`

### 2. 디코딩 (parsePacket)

```java
ParsedPacket parsed = protocol.parsePacket(packet);

int version = parsed.getProtocolVersion();
long packetLength = parsed.getPacketLength();
int fragmentFlag = parsed.getFragmentFlag();
int type = parsed.getPayloadType();
int user = parsed.getUserField();
byte[] payloadDecoded = parsed.getPayload();
```

`parsePacket`는 다음과 같은 상황에서 예외를 던집니다.

- 버퍼 길이가 최소 헤더+CRC(12바이트) 보다 작은 경우 → `BufferTooSmallException`
- 헤더에 기록된 `packetLength`가 최소 길이보다 작거나, `MAX_PACKET_LENGTH`를 초과하는 경우 → 각각 `BufferTooSmallException`, `PayloadTooLargeException`
- 헤더의 `packetLength`와 실제 `packetBytes.length`가 다른 경우 → `PacketSizeMismatch`
- 헤더+페이로드에 대한 CRC32 계산 결과가 마지막 4바이트와 다른 경우 → `InvalidCRCException`

## 샘플 코드

실행 가능한 예제는 `src/test/java/socketprotocol/SocketProtocolSample.java` 에 있습니다.

터미널에서 직접 실행하려면:

```bash
cd Low_Level_socket_protocol_Java
mvn compile
java -cp "target/test-classes;target/classes" socketprotocol.SocketProtocolSample
```

(Windows 기준; Unix 계열에서는 `:` 구분자를 사용하세요.)  
이 샘플은 문자열 페이로드를 인코딩/디코딩하고, 헤더 및 페이로드 정보를 콘솔에 출력합니다.

