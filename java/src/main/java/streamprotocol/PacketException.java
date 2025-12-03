package streamprotocol;

public class PacketException extends RuntimeException {
    public PacketException(String message) {
        super("PacketException: " + message);
    }
}
