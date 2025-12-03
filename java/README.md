# Low Level Stream Protocol (Java)

Java로 구현된 StreamProtocol 라이브러리입니다.  
순서가 보장되는 바이트 스트림(예: TCP, TLS 위의 스트림,
Unix 도메인 소켓, 파이프, 시리얼 링크 등) 위에서
바이너리 패킷을 인코딩/디코딩하기 위한 프레이밍 계층을 제공합니다.

## 프로토콜 개요

전체 패킷 레이아웃은 다음과 같습니다.

```text
[ 8바이트 헤더 ][ 페이로드 ][ 4바이트 CRC32 ]
```

- 헤더는 64비트 정수 하나를 little-endian 으로 표현한 것입니다.
- 헤더 비트 배열:
  - 비트 0–3  : `protocolVersion` (4비트)
  - 비트 4–48 : `packetLength` (45비트, 헤더 + 페이로드 + CRC 전체 길이)
  - 비트 49   : `fragmentFlag` (1비트)
  - 비트 50–53: `payloadType` (4비트)
  - 비트 54–63: `userField` (10비트)
- CRC32 는 다항식 `0xEDB88320` 을 사용합니다.

이 규칙만 지키면 어떤 순서 보장 바이트 스트림 위에서도 동일한 포맷을 사용할 수 있습니다.

## 주요 클래스

- `streamprotocol.StreamProtocol`
  - 패킷 인코딩/디코딩의 핵심 클래스입니다.
  - `toBytes(...)` : 페이로드를 패킷 바이트 배열로 변환.
  - `parsePacket(byte[])` : 패킷 바이트 배열을 파싱하여 `ParsedPacket` 으로 변환.
- `streamprotocol.ParsedPacket`
  - 파싱 결과를 담는 불변 객체입니다.
  - 헤더 필드(`protocolVersion`, `packetLength`, `fragmentFlag`,
    `payloadType`, `userField`)와 원본 페이로드(`byte[]`)를 제공합니다.
- 예외 계층
  - `PacketException` : 모든 프로토콜 관련 예외의 기본 클래스.
  - `BufferTooSmallException` : 헤더+CRC 를 담기에는 버퍼가 너무 작은 경우.
  - `PayloadTooLargeException` : 최대 패킷 크기를 초과하는 경우.
  - `PacketSizeMismatch` : 헤더에 기록된 길이와 실제 배열 길이가 다른 경우.
  - `InvalidCRCException` : CRC32 검증 실패.

## 기본 사용 예제

`StreamProtocolSample` 은 엔드투엔드 인코딩/디코딩 과정을 보여주는 간단한 샘플입니다.

```java
StreamProtocol protocol = new StreamProtocol();

byte[] payload = "Hello, TcpHelper!".getBytes();
byte fragFlag = StreamProtocol.UNFRAGED; // or FRAGED
byte payloadType = 0x01;                 // 0~15
int  userField   = 42;                   // 0~1023

// 패킷 인코딩
byte[] packet = protocol.toBytes(payload, fragFlag, payloadType, userField);

// 패킷 디코딩
ParsedPacket parsed = protocol.parsePacket(packet);

System.out.println("Protocol version: " + parsed.getProtocolVersion());
System.out.println("Packet length:    " + parsed.getPacketLength());
System.out.println("Fragment flag:    " + parsed.getFragmentFlag());
System.out.println("Payload type:     " + parsed.getPayloadType());
System.out.println("User field:       " + parsed.getUserField());
System.out.println("Payload string:   " + new String(parsed.getPayload()));
```

버퍼 크기를 제한하고 싶다면 다음 오버로드를 사용할 수 있습니다.

```java
int bufferSize = 1024;
byte[] boundedPacket =
        protocol.toBytes(payload, fragFlag, payloadType, userField, bufferSize);
```

## 빌드 및 테스트

Maven 프로젝트입니다.

```bash
cd java
mvn test
```

테스트 빌드 후, 샘플을 직접 실행하려면:

```bash
cd java
mvn -q -DskipTests=false test
java -cp "target/test-classes;target/classes" streamprotocol.StreamProtocolSample
```

(윈도우 환경 기준 클래스패스 예시입니다. 필요에 따라 `:`/`;` 구분자를 조정하세요.)
