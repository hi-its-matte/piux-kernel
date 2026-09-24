#ifndef HTTP_H
#define HTTP_H

#include <stdint.h>

/* Ultra-minimal HTTP/1.0 GET over the built-in TCP client. Strips the
   response headers and writes only the body into the given VFS file
   descriptor. Returns total body bytes written, or -1 on failure. */
int http_get_to_fd(uint32_t ip, uint16_t port, const char *path, const char *host, int output_fd);
int https_get_to_fd(uint32_t ip, uint16_t port, const char *path, const char *host, int output_fd);

#endif
