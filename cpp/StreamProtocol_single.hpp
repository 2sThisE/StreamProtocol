#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <limits>

/// 단일 헤더 버전 StreamProtocol C++ 구현입니다.
/// 이 파일 하나만 프로젝트에 포함하면 패킷 인코딩/디코딩을 사용할 수 있습니다.
namespace streamprotocol {

/// 모든 패킷 관련 예외의 기반 클래스입니다.
class PacketException : public std::runtime_error {
public:
    explicit PacketException(const std::string& msg)
        : std::runtime_error("PacketException: " + msg) {}
};

/// 인코딩 시 패킷 크기가 허용 가능한 최대 크기를 초과했을 때 던져집니다.
class PayloadTooLargeException : public PacketException {
public:
    explicit PayloadTooLargeException(size_t given, size_t max)
        : PacketException("Payload too large\n\tPayload Size: " + std::to_string(given) +
            "bytes\tBuffer Size:" + std::to_string(max) + "bytes") {
    }
};

/// 버퍼 크기가 최소 헤더+CRC 길이보다 작은 경우에 던져집니다.
class BufferTooSmallException : public PacketException {
public:
    explicit BufferTooSmallException(size_t given)
        : PacketException("Buffer size too small\n\tBuffer Size: " + std::to_string(given) + " bytes (min: 12bytes)") {
    }
};

/// 헤더에 기록된 길이와 실제 버퍼 길이가 다른 경우에 던져집니다.
class PacketSizeMismatch : public PacketException {
public:
    explicit PacketSizeMismatch(size_t bufferSize, size_t totalsize)
        : PacketException("Packet size mismatch: " + std::to_string(totalsize) + " bytes (Buffer: " + std::to_string(bufferSize) + ")") {
    }
};

/// CRC32 검증이 실패했을 때 던져집니다.
class InvalidCRCException : public PacketException {
public:
    explicit InvalidCRCException(uint32_t received, uint32_t computed)
        : PacketException("Invalid CRC checksum. Received: " + std::to_string(received) + ", Computed: " + std::to_string(computed)) {
    }
};

/// 파싱된 패킷 정보를 보관하는 불변 객체입니다.
class ParsedPacket {
private:
    uint8_t protocolVersion;
    size_t packetLength;
    uint8_t fragmentFlag;
    uint8_t payloadType;
    uint16_t userField;
    std::vector<uint8_t> payloadRaw;

public:
    ParsedPacket(uint8_t ver, size_t len, uint8_t frag, uint8_t type, uint16_t user, std::vector<uint8_t> payload)
        : protocolVersion(ver), packetLength(len), fragmentFlag(frag), payloadType(type), userField(user), payloadRaw(std::move(payload)) {
    }

    /// 프로토콜 버전(0~15)을 반환합니다.
    uint8_t ProtocolVersion() const { return protocolVersion; }
    /// 전체 패킷 길이(헤더 + 페이로드 + CRC)를 반환합니다.
    size_t PacketLength() const { return packetLength; }
    /// 단편화 플래그를 반환합니다.
    uint8_t FragmentFlag() const { return fragmentFlag; }
    /// 페이로드 타입(0~15)을 반환합니다.
    uint8_t PayloadType() const { return payloadType; }
    /// 사용자 필드 값(0~1023)을 반환합니다.
    uint16_t UserField() const { return userField; }
    /// 원본 페이로드 바이트를 반환합니다.
    const std::vector<uint8_t>& Payload() const { return payloadRaw; }
};

/// 8바이트 헤더 + CRC32를 사용하는 패킷 인코더/디코더입니다.
class StreamProtocol {
private:
    static constexpr size_t HEADER_SIZE = 8;               // 8 bytes
    static constexpr uint64_t MAX_HEADER_LENGTH_VALUE = 0x1FFFFFFFFFFFL; // 45-bit max

    uint8_t protocolVersion = 1; // Default protocol version (4-bit, 0-15)

    inline uint32_t computeCRC32(const uint8_t* data, size_t length) const {
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < length; ++i) {
            crc ^= data[i];
            for (int j = 0; j < 8; ++j) {
                uint32_t mask = 0 - (crc & 1);
                crc = (crc >> 1) ^ (0xEDB88320 & mask);
            }
        }
        return ~crc;
    }

    inline std::vector<uint8_t> buildPacket(const uint8_t* data, size_t size, uint8_t payloadType, uint8_t fragFlag, uint16_t userValue) const {
        if (data == nullptr) {
            throw std::invalid_argument("payload must not be null");
        }

        // Validate fragment flag
        if (fragFlag != FRAGED && fragFlag != UNFRAGED) {
            throw std::invalid_argument("Invalid fragment flag");
        }

        // Validate payload type (4-bit: 0-15)
        uint8_t pt = payloadType & 0xFF;
        if (pt > 0x0F) {
            throw std::invalid_argument("payloadType must be 4 bits (0-15)");
        }

        // Calculate total packet length (Header + Payload + CRC (4 bytes))
        uint64_t totalPacketLength64 = HEADER_SIZE + static_cast<uint64_t>(size) + sizeof(uint32_t);
        uint64_t maxPacketLength = (MAX_HEADER_LENGTH_VALUE < static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
            ? MAX_HEADER_LENGTH_VALUE
            : static_cast<uint64_t>(std::numeric_limits<size_t>::max());
        if (totalPacketLength64 > maxPacketLength) {
            throw PayloadTooLargeException(totalPacketLength64, maxPacketLength);
        }
        size_t totalPacketLength = static_cast<size_t>(totalPacketLength64);

        // Validate userField (10-bit: 0-1023)
        if (userValue > 0x3FF) {
            throw std::invalid_argument("userField must be 10-bit (0-1023)");
        }

        // Build 64-bit header value (little-endian)
        uint64_t headerValue = 0;
        headerValue |= (static_cast<uint64_t>(protocolVersion) & 0x0F) << 0;               // 4 bits
        headerValue |= (totalPacketLength64 & 0x1FFFFFFFFFFFull) << 4;                    // 45 bits
        headerValue |= (static_cast<uint64_t>(fragFlag) & 0x01u) << 49;                   // 1 bit
        headerValue |= (static_cast<uint64_t>(payloadType) & 0x0Fu) << 50;                // 4 bits
        headerValue |= (static_cast<uint64_t>(userValue) & 0x3FFu) << 54;                 // 10 bits

        std::vector<uint8_t> packet;
        packet.reserve(totalPacketLength);

        // Insert header (8 bytes, little-endian)
        for (size_t i = 0; i < HEADER_SIZE; ++i) {
            packet.push_back(static_cast<uint8_t>((headerValue >> (i * 8)) & 0xFFu));
        }

        // Insert payload data
        packet.insert(packet.end(), data, data + size);

        // Calculate CRC for header + payload
        uint32_t crc = computeCRC32(packet.data(), packet.size());
        const uint8_t* crcBytes = reinterpret_cast<const uint8_t*>(&crc);
        packet.insert(packet.end(), crcBytes, crcBytes + sizeof(uint32_t));

        return packet;
    }

public:
    static constexpr uint8_t FRAGED = 0x01;
    static constexpr uint8_t UNFRAGED = 0x00;

    /// 문자열 페이로드를 인코딩합니다.
    /// @param payload   전송할 문자열 데이터
    /// @param fragFlag  FRAGED / UNFRAGED
    /// @param userValue 10비트 사용자 필드 값 (0~1023)
    inline std::vector<uint8_t> toBytes(const std::string& payload, uint8_t fragFlag = UNFRAGED, uint16_t userValue = 0x00) const {
        return buildPacket(reinterpret_cast<const uint8_t*>(payload.data()), payload.size(), 0x01u, fragFlag, userValue);
    }

    /// 바이트 배열 페이로드를 인코딩합니다.
    inline std::vector<uint8_t> toBytes(const std::vector<uint8_t>& payload,
                                        uint8_t fragFlag = UNFRAGED,
                                        uint16_t userValue = 0x00) const {
        return buildPacket(payload.data(), payload.size(), 0x00u, fragFlag, userValue);
    }

    /// 인코딩된 패킷을 파싱하여 ParsedPacket으로 반환합니다.
    /// CRC/길이/버퍼 관련 검증에 실패하면 예외를 던집니다.
    inline ParsedPacket parsePacket(const std::vector<uint8_t>& packetBytes) const {
        if (packetBytes.size() < HEADER_SIZE + sizeof(uint32_t)) {
            throw BufferTooSmallException(packetBytes.size());
        }

        uint64_t maxPacketLength = (MAX_HEADER_LENGTH_VALUE < static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
            ? MAX_HEADER_LENGTH_VALUE
            : static_cast<uint64_t>(std::numeric_limits<size_t>::max());

        // Read 64-bit header (little-endian)
        uint64_t headerValue = 0;
        for (size_t i = 0; i < HEADER_SIZE; ++i) {
            headerValue |= (static_cast<uint64_t>(packetBytes[i]) << (i * 8));
        }

        uint8_t protoVersion = static_cast<uint8_t>((headerValue >> 0) & 0x0F);
        uint64_t packetLength64 = (headerValue >> 4) & 0x1FFFFFFFFFFFull;
        uint8_t fragmentFlag = static_cast<uint8_t>((headerValue >> 49) & 0x01);
        uint8_t payloadType = static_cast<uint8_t>((headerValue >> 50) & 0x0F);
        uint16_t userField = static_cast<uint16_t>((headerValue >> 54) & 0x3FF);

        if (packetLength64 < HEADER_SIZE + sizeof(uint32_t)) {
            throw BufferTooSmallException(packetLength64);
        }

        if (packetLength64 > maxPacketLength) {
            throw PayloadTooLargeException(packetLength64, maxPacketLength);
        }

        size_t packetLength = static_cast<size_t>(packetLength64);

        if (packetBytes.size() != packetLength) {
            throw PacketSizeMismatch(packetBytes.size(), packetLength);
        }

        // Extract received CRC
        uint32_t receivedCRC = 0;
        size_t crcOffset = packetLength - sizeof(uint32_t);
        for (int i = 0; i < 4; ++i) {
            receivedCRC |= (static_cast<uint32_t>(packetBytes[crcOffset + i]) << (i * 8));
        }

        uint32_t computedCRC = computeCRC32(packetBytes.data(), packetLength - sizeof(uint32_t));
        if (computedCRC != receivedCRC) {
            throw InvalidCRCException(receivedCRC, computedCRC);
        }

        std::vector<uint8_t> payload(packetBytes.begin() + HEADER_SIZE,
                                     packetBytes.begin() + packetLength - sizeof(uint32_t));

        return ParsedPacket(protoVersion, packetLength, fragmentFlag, payloadType, userField, std::move(payload));
    }

    /// 인코딩에 사용할 프로토콜 버전을 설정합니다. (0~15)
    inline void SetProtocolVersion(uint8_t version) {
        if (version > 0x0F) throw std::invalid_argument("Protocol version must be 4 bits (0-15)");
        protocolVersion = version;
    }
};

} // namespace streamprotocol
