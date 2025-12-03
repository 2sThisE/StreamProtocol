package streamprotocol;

public class PacketSizeMismatch extends PacketException {
    public PacketSizeMismatch(long bufferSize, long totalSize) {
        super("Packet size mismatch: " + totalSize + " bytes (Buffer: " + bufferSize + ")");
    }
}
