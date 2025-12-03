package streamprotocol;

public class BufferTooSmallException extends PacketException {
    public BufferTooSmallException(long given) {
        super("Buffer size too small. Buffer Size: " + given + " bytes (min: 12 bytes)");
    }
}
