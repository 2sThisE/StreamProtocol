package socketprotocol;

public class InvalidCRCException extends PacketException {
    public InvalidCRCException(long received, long computed) {
        super("Invalid CRC checksum. Received: " + received + ", Computed: " + computed);
    }
}
