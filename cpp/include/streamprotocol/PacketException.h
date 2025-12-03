#pragma once
#include <stdexcept>
#include <string>

namespace streamprotocol {

class PacketException : public std::runtime_error {
public:
    explicit PacketException(const std::string& msg)
        : std::runtime_error("PacketException: " + msg) {}
};

class PayloadTooLargeException : public PacketException {
public:
    explicit PayloadTooLargeException(size_t given, size_t max)
        : PacketException("Payload too large\n\tPayload Size: " + std::to_string(given) +
            "bytes\tBuffer Size:" + std::to_string(max)+"bytes") {
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
		: PacketException("Packet size mismatch: " + std::to_string(totalsize) + " bytes (Buffer: "+ std::to_string(bufferSize) +")") {
	}
};

class InvalidCRCException : public PacketException {
public:
    explicit InvalidCRCException(uint32_t received, uint32_t computed)
        : PacketException("Invalid CRC checksum. Received: " + std::to_string(received) +
                          ", Computed: " + std::to_string(computed)) {
    }
};

} // namespace streamprotocol
