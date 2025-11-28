#include <iostream>
#include "socketprotocol/SocketProtocol.hpp"

int main() {
    try {
        socketprotocol::SocketProtocol protocol;

        std::string message = "HelloPacket!";
        std::vector<uint8_t> packetBytes = protocol.toBytes(message, socketprotocol::SocketProtocol::UNFRAGED, 0x1f);

        std::cout << "Packet encoded! size: " << packetBytes.size() << " bytes" << std::endl;
        std::cout << "Packet (hex): ";
        for (uint8_t b : packetBytes) {
            printf("%02X ", b);
        }
        std::cout << std::endl;

        auto pkt = protocol.parsePacket(packetBytes);
        std::cout << "Protocol version: " << static_cast<int>(pkt.ProtocolVersion()) << std::endl;
        std::cout << "Total packet length: " << pkt.PacketLength() << std::endl;
        std::cout << "Payload type: " << static_cast<int>(pkt.PayloadType()) << std::endl;
        std::cout << "Fragment flag: " << static_cast<int>(pkt.FragmentFlag()) << std::endl;
        std::cout << "User field: " << pkt.UserField() << std::endl;

        std::string extracted(pkt.Payload().begin(), pkt.Payload().end());
        std::cout << "Extracted payload: " << extracted << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

