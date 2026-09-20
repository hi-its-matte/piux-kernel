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

static int net_string_equals(const char *left, const char *right) {
    while (*left && *right) {
        if (*left != *right) return 0;
        left++;
        right++;
    }
    return *left == *right;
}

int net_resolve_host(const char *text, uint32_t *out_ip) {
    int has_letters = 0;
    for (int index = 0; text[index]; index++) {
        unsigned char c = (unsigned char)text[index];
        if ((c >= '0' && c <= '9') || c == '.' || c == '-') {
            continue;
        }
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            has_letters = 1;
            continue;
        }
        return 0;
    }

    if (!has_letters) {
        *out_ip = net_parse_ip(text);
        return 1;
    }

    if (net_string_equals(text, "google.com") || net_string_equals(text, "www.google.com")) {
        *out_ip = net_parse_ip("8.8.8.8");
        return 1;
    }

    if (net_string_equals(text, "github.com") || net_string_equals(text, "www.github.com")) {
        *out_ip = net_parse_ip("140.82.121.4");
        return 1;
    }

    return 0;
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
