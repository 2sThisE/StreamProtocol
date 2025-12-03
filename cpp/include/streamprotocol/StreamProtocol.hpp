#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <limits>

#include "PacketException.h"
#include "ParsedPacket.hpp"

namespace streamprotocol {

class StreamProtocol {
private:
    static constexpr size_t HEADER_SIZE = 8;               // 8 bytes
    static constexpr uint64_t MAX_HEADER_LENGTH_VALUE = 0x1FFFFFFFFFFFL; // 45-bit max

    uint8_t protocolVersion = 1; // Default protocol version (4-bit, 0-15)

    uint32_t computeCRC32(const uint8_t* data, size_t length);
    std::vector<uint8_t> buildPacket(const uint8_t* data, size_t size, uint8_t payloadType, uint8_t fragFlag, uint16_t userValue);

public:
    static constexpr uint8_t FRAGED = 0x01;
    static constexpr uint8_t UNFRAGED = 0x00;

    static constexpr size_t MAX_PACKET_LENGTH = static_cast<size_t>(
        (MAX_HEADER_LENGTH_VALUE < static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
            ? MAX_HEADER_LENGTH_VALUE
            : static_cast<uint64_t>(std::numeric_limits<size_t>::max()));
    static constexpr size_t MAX_PAYLOAD_LENGTH = MAX_PACKET_LENGTH - HEADER_SIZE - sizeof(uint32_t);

    std::vector<uint8_t> toBytes(const std::string& payload, uint8_t fragFlag = UNFRAGED, uint16_t userValue = 0x00);
    std::vector<uint8_t> toBytes(const std::string& payload, uint8_t fragFlag, uint16_t userValue, size_t bufferSize);

    ParsedPacket parsePacket(const std::vector<uint8_t>& packetBytes);
    void SetProtocolVersion(uint8_t version);
};

} // namespace streamprotocol
