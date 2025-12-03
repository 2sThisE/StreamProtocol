package streamprotocol;

/**
 * Simple sample showing how to encode and decode a packet using {@link StreamProtocol}.
 * <p>
 * This is intended as an example for documentation / Git usage and is not used by production code.
 */
public class StreamProtocolSample {

    public static void main(String[] args) {
        StreamProtocol protocol = new StreamProtocol();

        // Example payload
        byte[] payload = "Hello, TcpHelper!".getBytes();

        // Fragment flag and payload type
        byte fragFlag = StreamProtocol.UNFRAGED;
        byte payloadType = 0x01; // application-defined type (0-15)
        int userField = 42;      // some user metadata (0-1023)

        // Encode
        byte[] packet = protocol.toBytes(payload, fragFlag, payloadType, userField);
        System.out.println("Encoded packet length: " + packet.length);

        // Decode
        ParsedPacket parsed = protocol.parsePacket(packet);

        System.out.println("Protocol version: " + parsed.getProtocolVersion());
        System.out.println("Packet length:    " + parsed.getPacketLength());
        System.out.println("Fragment flag:    " + parsed.getFragmentFlag());
        System.out.println("Payload type:     " + parsed.getPayloadType());
        System.out.println("User field:       " + parsed.getUserField());
        System.out.println("Payload string:   " + new String(parsed.getPayload()));

        // Example: encode with explicit buffer limit (will throw if too small)
        int bufferSize = 1024;
        byte[] boundedPacket = protocol.toBytes(payload, fragFlag, payloadType, userField, bufferSize);
        System.out.println("Encoded packet with buffer limit, length: " + boundedPacket.length);
    }
}
