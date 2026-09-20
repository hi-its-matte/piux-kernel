#include <stdint.h>
#include "net.h"

/* x86 is little-endian; network byte order is big-endian, so this is
   always a byte swap regardless of direction (host<->network). */
uint16_t net_htons(uint16_t value) {
    return (uint16_t)((value << 8) | (value >> 8));
}

uint32_t net_htonl(uint32_t value) {
    return ((value & 0x000000FFU) << 24) |
           ((value & 0x0000FF00U) << 8) |
           ((value & 0x00FF0000U) >> 8) |
           ((value & 0xFF000000U) >> 24);
}

uint16_t net_checksum(const void *data, uint32_t length, uint32_t initial_sum) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t sum = initial_sum;
    while (length > 1) {
        sum += (uint32_t)((bytes[0] << 8) | bytes[1]);
        bytes += 2;
        length -= 2;
    }
    if (length == 1) sum += (uint32_t)(bytes[0] << 8);
    while (sum >> 16) sum = (sum & 0xFFFFU) + (sum >> 16);
    return (uint16_t)(~sum & 0xFFFFU);
}

uint32_t net_parse_ip(const char *text) {
    uint32_t result = 0;
    uint32_t part = 0;
    int shift = 24;
    for (int index = 0; ; index++) {
        char character = text[index];
        if (character >= '0' && character <= '9') {
            part = part * 10 + (uint32_t)(character - '0');
        } else if (character == '.' || character == '\0') {
            result |= (part & 0xFFU) << shift;
            shift -= 8;
            part = 0;
            if (character == '\0') break;
        } else {
            break;
        }
    }
    return result;
}

void net_format_ip(uint32_t address, char *buffer) {
    int offset = 0;
    for (int shift = 24; shift >= 0; shift -= 8) {
        uint32_t octet = (address >> shift) & 0xFFU;
        if (octet >= 100) buffer[offset++] = (char)('0' + octet / 100);
        if (octet >= 10) buffer[offset++] = (char)('0' + (octet / 10) % 10);
        buffer[offset++] = (char)('0' + octet % 10);
        if (shift > 0) buffer[offset++] = '.';
    }
    buffer[offset] = '\0';
}
