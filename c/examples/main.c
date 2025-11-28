#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "socketprotocol/SocketProtocol.h"

int main(void) {
    const char* msg = "Hello, C SocketProtocol!";
    uint8_t* packet = NULL;
    size_t packet_len = 0;

    sp_result_t res = sp_encode_packet(
        (const uint8_t*)msg,
        strlen(msg),
        SP_UNFRAGED,
        0x01u,
        42u,
        &packet,
        &packet_len
    );

    if (res != SP_OK) {
        printf("encode failed: %d\n", (int)res);
        return 1;
    }

    printf("encoded packet length: %zu bytes\n", packet_len);

    sp_parsed_packet_t parsed;
    res = sp_parse_packet(packet, packet_len, &parsed);
    if (res != SP_OK) {
        printf("parse failed: %d\n", (int)res);
        free(packet);
        return 1;
    }

    printf("protocolVersion: %u\n", parsed.protocol_version);
    printf("packetLength:   %llu\n", (unsigned long long)parsed.packet_length);
    printf("fragmentFlag:   %u\n", parsed.fragment_flag);
    printf("payloadType:    %u\n", parsed.payload_type);
    printf("userField:      %u\n", parsed.user_field);

    printf("payload:        %.*s\n",
           (int)parsed.payload_length,
           (const char*)parsed.payload);

    free(packet);
    return 0;
}
