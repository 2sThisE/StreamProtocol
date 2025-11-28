#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <limits>

namespace socketprotocol {

class PacketException : public std::runtime_error {
public:
    explicit PacketException(const std::string& msg)
        : std::runtime_error("PacketException: " + msg) {}
};

class PayloadTooLargeException : public PacketException {
public:
    explicit PayloadTooLargeException(size_t given, size_t max)
        : PacketException("Payload too large\n\tPayload Size: " + std::to_string(given) +
            "bytes\tBuffer Size:" + std::to_string(max) + "bytes") {
    }
};

class BufferTooSmallException : public PacketException {
public:
    explicit BufferTooSmallException(size_t given)
        : PacketException("Buffer size too small\n\tBuffer Size: " + std::to_string(given) + " bytes (min: 12bytes)") {
    }
};

class PacketSizeMismatch : public PacketException {
public:
    explicit PacketSizeMismatch(size_t bufferSize, size_t totalsize)
        : PacketException("Packet size mismatch: " + std::to_string(totalsize) + " bytes (Buffer: " + std::to_string(bufferSize) + ")") {
    }
};

class InvalidCRCException : public PacketException {
public:
    explicit InvalidCRCException(uint32_t received, uint32_t computed)
        : PacketException("Invalid CRC checksum. Received: " + std::to_string(received) + ", Computed: " + std::to_string(computed)) {
    }
};

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

    uint8_t ProtocolVersion() const { return protocolVersion; }
    size_t PacketLength() const { return packetLength; }
    uint8_t FragmentFlag() const { return fragmentFlag; }
    uint8_t PayloadType() const { return payloadType; }
    uint16_t UserField() const { return userField; }
    const std::vector<uint8_t>& Payload() const { return payloadRaw; }
};

class SocketProtocol {
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

    inline std::vector<uint8_t> toBytes(const std::string& payload, uint8_t fragFlag = UNFRAGED, uint16_t userValue = 0x00) const {
        return buildPacket(reinterpret_cast<const uint8_t*>(payload.data()), payload.size(), 0x01u, fragFlag, userValue);
    }

    inline std::vector<uint8_t> toBytes(const std::vector<uint8_t>& payload, uint8_t fragFlag = UNFRAGED, uint16_t userValue = 0x00) const {
        return buildPacket(payload.data(), payload.size(), 0x00u, fragFlag, userValue);
    }

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

    inline void SetProtocolVersion(uint8_t version) {
        if (version > 0x0F) throw std::invalid_argument("Protocol version must be 4 bits (0-15)");
        protocolVersion = version;
    }
};

} // namespace socketprotocol

