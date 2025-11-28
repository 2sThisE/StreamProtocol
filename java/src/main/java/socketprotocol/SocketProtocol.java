package socketprotocol;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.zip.CRC32;

/**
 * 로우 레벨 TCP 패킷을 인코딩/디코딩하기 위한 유틸리티 클래스입니다.
 * <p>
 * 패킷 레이아웃:
 * <pre>
 * [ 8바이트 헤더 ][ 페이로드 ][ 4바이트 CRC32 ]
 * </pre>
 * 헤더는 64비트 little-endian 값이며, 비트 구성은 다음과 같습니다.
 * <ul>
 *     <li>비트 0-3  : protocolVersion (4비트)</li>
 *     <li>비트 4-48 : packetLength (45비트, 헤더 + 페이로드 + CRC 전체 길이)</li>
 *     <li>비트 49   : fragmentFlag (1비트)</li>
 *     <li>비트 50-53: payloadType (4비트)</li>
 *     <li>비트 54-63: userField (10비트)</li>
 * </ul>
 */
public class SocketProtocol {

    private byte protocolVersion = 1; // Default protocol version (4-bit, 0-15)

    public static final byte FRAGED = 0x01;
    public static final byte UNFRAGED = 0x00;

    // --- PacketHeader related constants and methods (moved from PacketHeader.java) ---
    // 64-bit header layout (little-endian):
    // bits 0-3   : protocolVersion (4 bits)
    // bits 4-48  : packetLength    (45 bits, header + payload + CRC)
    // bit  49    : fragmentFlag    (1 bit)
    // bits 50-53 : payloadType     (4 bits)
    // bits 54-63 : userField       (10 bits)
    private static final int HEADER_SIZE = 8; // 8 bytes
    private static final long MAX_HEADER_LENGTH_VALUE = 0x1FFFFFFFFFFFL; // 45-bit max
    /** 이 구현에서 지원하는 최대 패킷 길이 (헤더 + 페이로드 + CRC 포함)입니다. */
    public static final int MAX_PACKET_LENGTH = (int) Math.min(MAX_HEADER_LENGTH_VALUE, Integer.MAX_VALUE);
    /** 인코딩 가능한 최대 페이로드 길이입니다. (헤더/CRC 제외) */
    public static final int MAX_PAYLOAD_LENGTH = MAX_PACKET_LENGTH - HEADER_SIZE - 4; // exclude header + CRC

    private static long getProtocolVersion(ByteBuffer buffer) {
        long headerValue = buffer.getLong(0);
        return (headerValue >> 0) & 0xF; // 4 bits
    }

    private static long getPacketLength(ByteBuffer buffer) {
        long headerValue = buffer.getLong(0);
        // 45 bits for packet length (bits 4-48)
        return (headerValue >> 4) & 0x1FFFFFFFFFFFL;
    }

    private static long getFragmentFlag(ByteBuffer buffer) {
        long headerValue = buffer.getLong(0);
        return (headerValue >> 49) & 0x1; // 1 bit
    }

    private static long getPayloadType(ByteBuffer buffer) {
        long headerValue = buffer.getLong(0);
        return (headerValue >> 50) & 0xF; // 4 bits
    }

    private static long getUserField(ByteBuffer buffer) {
        long headerValue = buffer.getLong(0);
        return (headerValue >> 54) & 0x3FF; // 10 bits
    }

    private static byte[] buildHeader(byte protocolVersion, long packetLength, byte fragmentFlag, byte payloadType, int userField) {
        long headerValue = 0L;
        headerValue |= ((long) protocolVersion & 0xFL) << 0; // 4 bits
        headerValue |= (packetLength & 0x1FFFFFFFFFFFL) << 4; // 45 bits
        headerValue |= ((long) fragmentFlag & 0x1L) << 49; // 1 bit
        headerValue |= ((long) payloadType & 0xFL) << 50; // 4 bits
        headerValue |= ((long) userField & 0x3FFL) << 54; // 10 bits

        ByteBuffer buffer = ByteBuffer.allocate(HEADER_SIZE);
        buffer.order(ByteOrder.LITTLE_ENDIAN); // Match C++ packing
        buffer.putLong(headerValue);
        return buffer.array();
    }
    // --- End of PacketHeader related methods ---

    // CRC32 계산 (java.util.zip.CRC32 사용)
    private int computeCRC32(byte[] data, int length) {
        CRC32 crc32 = new CRC32();
        crc32.update(data, 0, length);
        return (int) crc32.getValue();
    }

    private byte[] buildPacket(byte[] data, int size, byte payloadType, byte fragFlag, int userValue) {
        if (data == null) {
            throw new IllegalArgumentException("payload must not be null");
        }

        // fragment flag 검증
        if (fragFlag != FRAGED && fragFlag != UNFRAGED) {
            throw new IllegalArgumentException("Invalid fragment flag: " + fragFlag);
        }

        // payload type 검증 (4비트: 0-15)
        int pt = payloadType & 0xFF;
        if (pt < 0 || pt > 0x0F) {
            throw new IllegalArgumentException("payloadType must be 4 bits (0-15): " + pt);
        }

        // 전체 패킷 길이 계산 (헤더 + 페이로드 + CRC(4바이트))
        long totalPacketLengthLong = HEADER_SIZE + (long) size + 4L;

        // 헤더 비트 폭 및 Java 배열 한계를 기준으로 최대 패킷 길이 검증
        if (totalPacketLengthLong > MAX_PACKET_LENGTH) {
            throw new PayloadTooLargeException(totalPacketLengthLong, MAX_PACKET_LENGTH);
        }

        int totalPacketLength = (int) totalPacketLengthLong;

        // userField 검증 (10비트: 0-1023)
        if (userValue < 0 || userValue > 0x3FF) {
            throw new IllegalArgumentException("userField must be 10-bit (0-1023)");
        }

        // Build header bytes
        byte[] headerBytes = buildHeader(
                protocolVersion,
                totalPacketLength,
                fragFlag,
                payloadType,
                userValue
        );

        // 패킷 버퍼 생성
        ByteBuffer packetBuffer = ByteBuffer.allocate(totalPacketLength);
        packetBuffer.order(ByteOrder.LITTLE_ENDIAN); // Match C++ packing

        // 헤더 삽입
        packetBuffer.put(headerBytes);

        // 페이로드 데이터 삽입
        packetBuffer.put(data, 0, size);

        // 헤더 + 페이로드에 대한 CRC 계산
        byte[] crcData = new byte[HEADER_SIZE + size];
        System.arraycopy(packetBuffer.array(), 0, crcData, 0, HEADER_SIZE + size);
        int crc = computeCRC32(crcData, crcData.length);

        // CRC 삽입
        packetBuffer.putInt(crc);

        return packetBuffer.array();
    }

    /**
     * 주어진 페이로드를 프로토콜 헤더와 CRC32를 사용하여 패킷으로 인코딩합니다.
     *
     * @param payload    애플리케이션 페이로드 바이트 (null이면 안 됨)
     * @param fragFlag   단편화 플래그, {@link #FRAGED} 또는 {@link #UNFRAGED} 여야 합니다.
     * @param payloadType 논리적 페이로드 타입 (0~15)
     * @param userValue  10비트 사용자 필드 값 (0~1023)
     * @return 인코딩된 패킷 바이트 배열
     * @throws IllegalArgumentException 인자가 허용 범위를 벗어난 경우
     * @throws PayloadTooLargeException 결과 패킷이 {@link #MAX_PACKET_LENGTH}를 초과하는 경우
     */
    public byte[] toBytes(byte[] payload, byte fragFlag, byte payloadType ,int userValue) {
        return buildPacket(payload, payload.length, payloadType, fragFlag, userValue);
    }

    /**
     * 추가로 호출자가 지정한 최대 패킷 크기를 사용하여 인코딩합니다.
     * <p>
     * 결과 패킷이 {@code bufferSize}를 초과하지 않는지 먼저 확인한 뒤,
     * {@link #toBytes(byte[], byte, byte, int)}와 동일한 검증을 수행합니다.
     *
     * @param payload    애플리케이션 페이로드 바이트 (null이면 안 됨)
     * @param fragFlag   단편화 플래그, {@link #FRAGED} 또는 {@link #UNFRAGED}
     * @param payloadType 논리적 페이로드 타입 (0~15)
     * @param userValue  10비트 사용자 필드 값 (0~1023)
     * @param bufferSize 허용할 최대 전체 패킷 크기 (바이트 단위)
     * @return 인코딩된 패킷 바이트 배열
     * @throws BufferTooSmallException  {@code bufferSize}가 최소 헤더+CRC 길이보다 작은 경우
     * @throws IllegalArgumentException 인자가 허용 범위를 벗어난 경우
     * @throws PayloadTooLargeException 결과 패킷이 {@code bufferSize}를 초과하는 경우
     */
    public byte[] toBytes(byte[] payload, byte fragFlag, byte payloadType ,int userValue, int bufferSize) {
        if (bufferSize < HEADER_SIZE + 4) {
            throw new BufferTooSmallException(bufferSize);
        }

        long totalPacketLengthLong = HEADER_SIZE + (long) payload.length + 4L;
        if (totalPacketLengthLong > bufferSize) {
            throw new PayloadTooLargeException(totalPacketLengthLong, bufferSize);
        }

        return buildPacket(payload, payload.length, payloadType, fragFlag, userValue);
    }

    /**
     * 인코딩된 패킷 바이트 배열을 {@link ParsedPacket}으로 파싱합니다.
     * 헤더, 길이, CRC를 모두 검증합니다.
     *
     * @param packetBytes 헤더 + 페이로드 + CRC를 포함한 원시 패킷 바이트 배열
     * @return 파싱된 패킷 표현 객체
     * @throws BufferTooSmallException 버퍼가 헤더와 CRC를 담기에 너무 작은 경우
     * @throws PacketSizeMismatch      버퍼 길이가 헤더에 기록된 길이와 다른 경우
     * @throws PayloadTooLargeException 헤더의 길이 필드가 {@link #MAX_PACKET_LENGTH}를 초과하는 경우
     * @throws InvalidCRCException     CRC32 체크섬 검증에 실패한 경우
     */
    public ParsedPacket parsePacket(byte[] packetBytes) {
        if (packetBytes.length < HEADER_SIZE + 4) {
            throw new BufferTooSmallException(packetBytes.length);
        }

        ByteBuffer buffer = ByteBuffer.wrap(packetBytes);
        buffer.order(ByteOrder.LITTLE_ENDIAN); // Match C++ packing

        // 헤더 필드 추출
        long protoVersion = getProtocolVersion(buffer);
        long packetLength = getPacketLength(buffer);
        long fragmentFlag = getFragmentFlag(buffer);
        long payloadType = getPayloadType(buffer);
        long userField = getUserField(buffer);

        // 헤더로부터 읽어낸 길이 필드 검증
        if (packetLength < HEADER_SIZE + 4) {
            throw new BufferTooSmallException(packetLength);
        }

        if (packetLength > MAX_PACKET_LENGTH) {
            throw new PayloadTooLargeException(packetLength, MAX_PACKET_LENGTH);
        }

        // 실제 패킷 길이와 헤더의 길이 필드 비교
        if (packetBytes.length != packetLength) {
            throw new PacketSizeMismatch(packetBytes.length, packetLength);
        }

        // 수신된 CRC 추출 (패킷 마지막 4바이트)
        int receivedCRC = buffer.getInt((int) packetLength - 4);

        // 헤더 + 페이로드에 대해 CRC 계산 (수신된 CRC 제외)
        byte[] crcData = new byte[(int) packetLength - 4];
        System.arraycopy(packetBytes, 0, crcData, 0, (int) packetLength - 4);
        int computedCRC = computeCRC32(crcData, crcData.length);

        // CRC 검증
        if (computedCRC != receivedCRC) {
            throw new InvalidCRCException(receivedCRC, computedCRC);
        }

        // 페이로드 추출
        byte[] payload = new byte[(int) packetLength - HEADER_SIZE - 4]; // Total - Header - CRC
        buffer.position(HEADER_SIZE); // Move past header
        buffer.get(payload); // Read payload bytes

        return new ParsedPacket(
                (int) protoVersion,
                packetLength,
                (int) fragmentFlag,
                (int) payloadType,
                (int) userField,
                payload
        );
    }

    /**
     * 새 패킷을 인코딩할 때 사용할 프로토콜 버전을 설정합니다.
     *
     * @param version 프로토콜 버전 (0~15)
     * @throws IllegalArgumentException {@code version}이 4비트 범위를 벗어난 경우
     */
    public void setProtocolVersion(byte version) {
        if ((version & 0xFF) > 0x0F) { // Ensure unsigned comparison
            throw new IllegalArgumentException("Protocol version must be 4 bits (0-15)");
        }
        this.protocolVersion = version;
    }
}
