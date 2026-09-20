#ifndef NET_H
#define NET_H

#include <stdint.h>

uint16_t net_htons(uint16_t value);
uint32_t net_htonl(uint32_t value);
uint16_t net_checksum(const void *data, uint32_t length, uint32_t initial_sum);
uint32_t net_parse_ip(const char *text);
int net_resolve_host(const char *text, uint32_t *out_ip);
void net_format_ip(uint32_t address, char *buffer);

#endif
