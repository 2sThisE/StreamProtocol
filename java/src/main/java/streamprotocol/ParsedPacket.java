package streamprotocol;

/**
 * 파싱된 패킷을 표현하는 불변 객체입니다.
 * <p>
 * {@link StreamProtocol#parsePacket(byte[])} 호출 결과로 생성되며,
 * 헤더 필드와 원본 페이로드 바이트를 제공합니다.
 */
public class ParsedPacket {
    private final int protocolVersion;
    private final long packetLength;
    private final int fragmentFlag;
    private final int payloadType;
    private final int userField;
    private final byte[] payloadRaw;

    /**
     * 파싱된 패킷 인스턴스를 생성합니다.
     *
     * @param protocolVersion 헤더에서 추출된 프로토콜 버전 (0~15)
     * @param packetLength    전체 패킷 길이 (바이트 단위, 헤더 + 페이로드 + CRC)
     * @param fragmentFlag    단편화 플래그 ({@link StreamProtocol#FRAGED} / {@link StreamProtocol#UNFRAGED})
     * @param payloadType     논리적 페이로드 타입 (0~15)
     * @param userField       사용자 필드 값 (0~1023)
     * @param payloadRaw      원본 페이로드 바이트 (null 아님)
     */
    public ParsedPacket(int protocolVersion, long packetLength, int fragmentFlag, int payloadType, int userField, byte[] payloadRaw) {
        this.protocolVersion = protocolVersion;
        this.packetLength = packetLength;
        this.fragmentFlag = fragmentFlag;
        this.payloadType = payloadType;
        this.userField = userField;
        this.payloadRaw = payloadRaw;
    }

    /** 프로토콜 버전 필드(0~15)를 반환합니다. */
    public int getProtocolVersion() {
        return protocolVersion;
    }

    /** 전체 패킷 길이(헤더 + 페이로드 + CRC)를 반환합니다. */
    public long getPacketLength() {
        return packetLength;
    }

    /** 단편화 플래그 값을 반환합니다. */
    public int getFragmentFlag() {
        return fragmentFlag;
    }

    /** 논리적 페이로드 타입(0~15)을 반환합니다. */
    public int getPayloadType() {
        return payloadType;
    }

    /** 사용자 정의 필드 값(0~1023)을 반환합니다. */
    public int getUserField() {
        return userField;
    }

    /** 원본 페이로드 바이트 배열을 반환합니다. */
    public byte[] getPayload() {
        return payloadRaw;
    }
}
