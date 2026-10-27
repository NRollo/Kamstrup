#ifndef KMP_CODEC_H
#define KMP_CODEC_H

/* Kamstrup Meter Protocol (KMP) framing and decoding, independent of ESP-IDF. */

#include <stdint.h>
#include <stddef.h>

#define KMP_REQ_START        0x80
#define KMP_RESP_START       0x40
#define KMP_EOM              0x0D
#define KMP_ESC              0x1B
#define KMP_ADDR_HEAT_METER  0x3F
#define KMP_CID_GET_REGISTER 0x10

#define KMP_MAX_REQUEST_LEN  16
#define KMP_MAX_FRAME_LEN    40

typedef struct {
    uint16_t reg;
    double value;
    uint8_t unit;
} kmp_value_t;

typedef enum {
    KMP_OK = 0,
    KMP_ERR_FRAME,
    KMP_ERR_CRC,
    KMP_ERR_HEADER,
    KMP_ERR_REGISTER,
    KMP_ERR_PAYLOAD,
} kmp_status_t;

/* CRC-16/XMODEM (poly 0x1021, init 0, no reflection). */
uint16_t kmp_crc16(const uint8_t *data, size_t len);

/* Builds a stuffed GetRegister request for one register. Returns 0 if out_size is too small. */
size_t kmp_build_request(uint16_t reg, uint8_t *out, size_t out_size);

/*
 * Decodes a response. frame holds the bytes after the 0x40 start byte,
 * up to and including the 0x0D end byte.
 */
kmp_status_t kmp_parse_response(const uint8_t *frame, size_t len, uint16_t expected_reg, kmp_value_t *out);

const char *kmp_unit_name(uint8_t unit);

#endif
