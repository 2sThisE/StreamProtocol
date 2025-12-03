#include "streamprotocol/StreamProtocol.hpp"

namespace streamprotocol {

uint32_t StreamProtocol::computeCRC32(const uint8_t* data, size_t length) {
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

std::vector<uint8_t> StreamProtocol::buildPacket(const uint8_t* data, size_t size, uint8_t payloadType, uint8_t fragFlag, uint16_t userValue) {
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
    if (totalPacketLength64 > MAX_PACKET_LENGTH) {
        throw PayloadTooLargeException(totalPacketLength64, MAX_PACKET_LENGTH);
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
    for (int i = 0; i < static_cast<int>(HEADER_SIZE); ++i) {
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

std::vector<uint8_t> StreamProtocol::toBytes(const std::string& payload, uint8_t fragFlag, uint16_t userValue) {
    return buildPacket(reinterpret_cast<const uint8_t*>(payload.data()), payload.size(), 0x01u, fragFlag, userValue);
}

std::vector<uint8_t> StreamProtocol::toBytes(const std::string& payload, uint8_t fragFlag, uint16_t userValue, size_t bufferSize) {
    if (bufferSize < HEADER_SIZE + sizeof(uint32_t)) {
        throw BufferTooSmallException(bufferSize);
    }

    uint64_t totalPacketLength64 = HEADER_SIZE + static_cast<uint64_t>(payload.size()) + sizeof(uint32_t);
    if (totalPacketLength64 > bufferSize) {
        throw PayloadTooLargeException(totalPacketLength64, bufferSize);
    }

    return buildPacket(reinterpret_cast<const uint8_t*>(payload.data()), payload.size(), 0x01u, fragFlag, userValue);
}

ParsedPacket StreamProtocol::parsePacket(const std::vector<uint8_t>& packetBytes) {
    if (packetBytes.size() < HEADER_SIZE + sizeof(uint32_t)) {
        throw BufferTooSmallException(packetBytes.size());
    }

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

    if (packetLength64 > MAX_PACKET_LENGTH) {
        throw PayloadTooLargeException(packetLength64, MAX_PACKET_LENGTH);
    }

    size_t packetLength = static_cast<size_t>(packetLength64);

    // Validate packet length against actual buffer size
    if (packetBytes.size() != packetLength) {
        throw PacketSizeMismatch(packetBytes.size(), packetLength);
    }

    // Extract received CRC (last 4 bytes)
    uint32_t receivedCRC = 0;
    size_t crcOffset = packetLength - sizeof(uint32_t);
    for (int i = 0; i < 4; ++i) {
        receivedCRC |= (static_cast<uint32_t>(packetBytes[crcOffset + i]) << (i * 8));
    }

    // Compute CRC for header + payload (excluding CRC itself)
    uint32_t computedCRC = computeCRC32(packetBytes.data(), packetLength - sizeof(uint32_t));
    if (computedCRC != receivedCRC) {
        throw InvalidCRCException(receivedCRC, computedCRC);
    }

    // Extract payload
    std::vector<uint8_t> payload(packetBytes.begin() + HEADER_SIZE,
                                 packetBytes.begin() + packetLength - sizeof(uint32_t));

    return ParsedPacket(protoVersion, packetLength, fragmentFlag, payloadType, userField, std::move(payload));
}

void StreamProtocol::SetProtocolVersion(uint8_t version) {
    if (version > 0x0F) {
        throw std::invalid_argument("Protocol version must be 4 bits (0-15)");
    }
    protocolVersion = version;
}

} // namespace streamprotocol
