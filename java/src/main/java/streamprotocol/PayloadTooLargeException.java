package streamprotocol;

public class PayloadTooLargeException extends PacketException {
    public PayloadTooLargeException(long given, long max) {
        super("Payload too large. Packet size: " + given +
              " bytes (max: " + max + " bytes)");
    }
}
