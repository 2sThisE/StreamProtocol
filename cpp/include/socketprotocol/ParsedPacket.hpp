#pragma once
#include <cstdint>
#include <vector>

namespace socketprotocol {

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

} // namespace socketprotocol

