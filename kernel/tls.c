#include <stdint.h>
#include <stddef.h>
#include "tls.h"
#include "tcp.h"
#include "../third_party/bearssl/inc/bearssl.h"

extern const br_x509_trust_anchor TAs[];
#define TAs_NUM 3

static br_ssl_client_context client;
static br_x509_minimal_context x509;
static uint8_t input_buffer[BR_SSL_BUFSIZE_INPUT];
static uint8_t output_buffer[BR_SSL_BUFSIZE_OUTPUT];
static int closed;

static int tls_seed(uint32_t *seed) {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t max_leaf;
    uint8_t success;

    __asm__ __volatile__("cpuid" : "=a"(max_leaf), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    if (max_leaf < 1) return -1;
    __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    if (!(ecx & (1U << 30))) return -1;
    __asm__ __volatile__(".byte 0x0f, 0xc7, 0xf0; setc %1" : "=a"(*seed), "=qm"(success));
    return success ? 0 : -1;
}

static int tls_flush_records(void) {
    size_t length;
    unsigned char *record;
    record = br_ssl_engine_sendrec_buf(&client.eng, &length);
    while (record != NULL && length > 0) {
        if (tcp_send(record, (uint16_t)length) < 0) return -1;
        br_ssl_engine_sendrec_ack(&client.eng, length);
        record = br_ssl_engine_sendrec_buf(&client.eng, &length);
    }
    return 0;
}

static int tls_pump(void) {
    size_t length;
    unsigned char *record;
    uint8_t received[1460];
    int count;

    if (tls_flush_records() < 0) return -1;
    record = br_ssl_engine_recvrec_buf(&client.eng, &length);
    if (record == NULL || length == 0) return 0;
    count = tcp_receive(received, sizeof(received));
    if (count > 0) {
        if ((size_t)count > length) count = (int)length;
        for (int index = 0; index < count; index++) record[index] = received[index];
        br_ssl_engine_recvrec_ack(&client.eng, (size_t)count);
    }
    if (tls_flush_records() < 0) return -1;
    if (tcp_is_closed()) closed = 1;
    return count;
}

int tls_connect(uint32_t ip, uint16_t port, const char *server_name) {
    uint32_t seed[4];
    unsigned state;

    if (tcp_connect(ip, port) < 0) return -1;
    br_ssl_client_init_full(&client, &x509, TAs, TAs_NUM);
    br_ssl_engine_set_buffers_bidi(&client.eng, input_buffer, sizeof(input_buffer), output_buffer, sizeof(output_buffer));
    for (int index = 0; index < 4; index++) if (tls_seed(&seed[index]) < 0) return -1;
    br_ssl_engine_inject_entropy(&client.eng, seed, sizeof(seed));
    if (!br_ssl_client_reset(&client, server_name, 0)) return -1;

    for (int attempt = 0; attempt < 500000; attempt++) {
        state = br_ssl_engine_current_state(&client.eng);
        if (state & BR_SSL_CLOSED) return -1;
        if (state & BR_SSL_SENDAPP) return 0;
        if (tls_pump() < 0) return -1;
    }
    return -1;
}

int tls_send(const void *data, uint16_t length) {
    size_t available;
    unsigned char *application;
    application = br_ssl_engine_sendapp_buf(&client.eng, &available);
    if (application == NULL || available < length) return -1;
    for (uint16_t index = 0; index < length; index++) application[index] = ((const uint8_t *)data)[index];
    br_ssl_engine_sendapp_ack(&client.eng, length);
    return tls_flush_records();
}

int tls_receive(uint8_t *buffer, uint16_t max_length) {
    size_t available;
    unsigned char *application;
    for (int attempt = 0; attempt < 300000; attempt++) {
        application = br_ssl_engine_recvapp_buf(&client.eng, &available);
        if (application != NULL && available > 0) {
            if (available > max_length) available = max_length;
            for (size_t index = 0; index < available; index++) buffer[index] = application[index];
            br_ssl_engine_recvapp_ack(&client.eng, available);
            return (int)available;
        }
        if (tls_pump() < 0) return -1;
        if (closed) return 0;
    }
    return 0;
}

int tls_is_closed(void) { return closed || (br_ssl_engine_current_state(&client.eng) & BR_SSL_CLOSED); }

int tls_last_error(void) { return (int)br_ssl_engine_last_error(&client.eng); }

void tls_close(void) {
    br_ssl_engine_close(&client.eng);
    tls_flush_records();
    tcp_close();
    closed = 1;
}