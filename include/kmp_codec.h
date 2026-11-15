#ifndef KMP_CODEC_H
#define KMP_CODEC_H

/**
 * @file kmp_codec.h
 * @brief Kamstrup Meter Protocol (KMP) framing and decoding, independent of ESP-IDF.
 */

#include <stdint.h>
#include <stddef.h>

#define KMP_REQ_START        0x80 /**< Start byte of a request frame (master to meter). */
#define KMP_RESP_START       0x40 /**< Start byte of a response frame (meter to master). */
#define KMP_EOM              0x0D /**< End-of-message byte. */
#define KMP_ESC              0x1B /**< Escape byte used for byte stuffing. */
#define KMP_ADDR_HEAT_METER  0x3F /**< Destination address of a heat meter. */
#define KMP_CID_GET_REGISTER 0x10 /**< Command ID: GetRegister. */

#define KMP_MAX_REQUEST_LEN  16 /**< Buffer size that always fits a stuffed GetRegister request. */
#define KMP_MAX_FRAME_LEN    40 /**< Maximum size of a response frame (after the start byte) that is accepted. */

/** @brief A decoded register value. */
typedef struct {
    uint16_t reg;   /**< Register number the value was read from. */
    double value;   /**< Value with sign and decimal exponent applied. */
    uint8_t unit;   /**< KMP unit code, see kmp_unit_name(). */
} kmp_value_t;

/** @brief Result of decoding a response frame. */
typedef enum {
    KMP_OK = 0,        /**< Frame decoded successfully. */
    KMP_ERR_FRAME,     /**< Malformed frame (missing end byte, bad escape, too short or too long). */
    KMP_ERR_CRC,       /**< CRC mismatch. */
    KMP_ERR_HEADER,    /**< Unexpected address or command ID. */
    KMP_ERR_REGISTER,  /**< Response is for a different register than requested. */
    KMP_ERR_PAYLOAD,   /**< Invalid mantissa length or payload size. */
} kmp_status_t;

/**
 * @brief Calculates the CRC-16/XMODEM checksum (poly 0x1021, init 0, no reflection).
 *
 * @param data Bytes to checksum.
 * @param len  Number of bytes in @p data.
 * @return The 16-bit CRC.
 */
uint16_t kmp_crc16(const uint8_t *data, size_t len);

/**
 * @brief Builds a byte-stuffed GetRegister request for one register.
 *
 * The frame is `0x80 | dest addr | CID | register count | register (2 bytes) | CRC-16 (2 bytes) | 0x0D`.
 * The CRC covers the bytes between the start byte and the CRC itself. Any control byte inside the
 * message (0x06, 0x0D, 0x1B, 0x40, 0x80) is sent as 0x1B followed by the byte XOR 0xFF.
 *
 * @param reg      Register number to read.
 * @param out      Buffer that receives the complete frame, including start and end bytes.
 * @param out_size Size of @p out in bytes; KMP_MAX_REQUEST_LEN is always enough.
 * @return Number of bytes written, or 0 if @p out_size is too small.
 */
size_t kmp_build_request(uint16_t reg, uint8_t *out, size_t out_size);

/**
 * @brief Decodes a GetRegister response.
 *
 * Removes the byte stuffing, verifies the CRC, address, command ID and register number, then
 * converts the payload to `value = +/- mantissa * 10^(+/- exponent)`. The checks run in that order,
 * so the returned status shows the first thing that was wrong.
 *
 * @param frame        Bytes after the 0x40 start byte, up to and including the 0x0D end byte.
 * @param len          Number of bytes in @p frame.
 * @param expected_reg Register that was requested; the response must match it.
 * @param[out] out     Receives the decoded value when KMP_OK is returned.
 * @return KMP_OK on success, otherwise the reason the frame was rejected.
 */
kmp_status_t kmp_parse_response(const uint8_t *frame, size_t len, uint16_t expected_reg, kmp_value_t *out);

/**
 * @brief Converts a KMP unit code to a printable unit string.
 *
 * @param unit KMP unit code.
 * @return Unit string such as "kWh" or "m3/h", or "???" if the code is unknown.
 */
const char *kmp_unit_name(uint8_t unit);

#endif
