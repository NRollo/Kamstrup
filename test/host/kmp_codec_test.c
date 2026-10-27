/*
 * Host-side test for the KMP codec. Expected frames were generated with an
 * independent Python implementation of CRC-16/XMODEM and KMP byte stuffing.
 *
 * Run from the project root:
 *   gcc -Wall -Werror -Iinclude test/host/kmp_codec_test.c src/kmp_codec.c -lm -o kmp_codec_test && kmp_codec_test
 */
#include "kmp_codec.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            printf("FAIL line %d: %s\n", __LINE__, #cond);             \
            failures++;                                                \
        }                                                              \
    } while (0)

static void test_crc(void) {
    CHECK(kmp_crc16((const uint8_t *)"123456789", 9) == 0x31C3);
}

static void test_request(void) {
    uint8_t out[KMP_MAX_REQUEST_LEN];
    const uint8_t expected_plain[] = {0x80, 0x3F, 0x10, 0x01, 0x00, 0x3C, 0xB2, 0x5F, 0x0D};
    size_t n = kmp_build_request(0x003C, out, sizeof(out));
    CHECK(n == sizeof(expected_plain));
    CHECK(memcmp(out, expected_plain, sizeof(expected_plain)) == 0);

    const uint8_t expected_stuffed[] = {0x80, 0x3F, 0x10, 0x01, 0x1B, 0xF9, 0x1B, 0xBF, 0xA7, 0xE2, 0x0D};
    n = kmp_build_request(0x0640, out, sizeof(out));
    CHECK(n == sizeof(expected_stuffed));
    CHECK(memcmp(out, expected_stuffed, sizeof(expected_stuffed)) == 0);

    CHECK(kmp_build_request(0x003C, out, 4) == 0);
}

static void test_response_stuffed(void) {
    const uint8_t frame[] = {0x3F, 0x10, 0x00, 0x3C, 0x08, 0x04, 0x43, 0x00, 0x1B, 0xF2, 0x1B,
                             0xBF, 0x1B, 0x7F, 0x03, 0x2A, 0x0D};
    kmp_value_t v;
    CHECK(kmp_parse_response(frame, sizeof(frame), 0x003C, &v) == KMP_OK);
    CHECK(v.reg == 0x003C);
    CHECK(strcmp(kmp_unit_name(v.unit), "GJ") == 0);
    CHECK(fabs(v.value - 868.480) < 1e-9);
}

static void test_response_plain_and_negative(void) {
    const uint8_t temp[] = {0x3F, 0x10, 0x00, 0x57, 0x25, 0x04, 0x41, 0x00, 0x00, 0x01, 0x9A, 0x57, 0x95, 0x0D};
    const uint8_t neg[] = {0x3F, 0x10, 0x00, 0x59, 0x26, 0x02, 0xC1, 0x00, 0x64, 0x7C, 0x28, 0x0D};
    kmp_value_t v;
    CHECK(kmp_parse_response(temp, sizeof(temp), 0x0057, &v) == KMP_OK);
    CHECK(fabs(v.value - 41.0) < 1e-9);
    CHECK(strcmp(kmp_unit_name(v.unit), "C") == 0);
    CHECK(kmp_parse_response(neg, sizeof(neg), 0x0059, &v) == KMP_OK);
    CHECK(fabs(v.value + 10.0) < 1e-9);
}

static void test_errors(void) {
    uint8_t frame[] = {0x3F, 0x10, 0x00, 0x57, 0x25, 0x04, 0x41, 0x00, 0x00, 0x01, 0x9A, 0x57, 0x95, 0x0D};
    kmp_value_t v;

    CHECK(kmp_parse_response(frame, sizeof(frame), 0x0050, &v) == KMP_ERR_REGISTER);

    frame[8] ^= 0x01;
    CHECK(kmp_parse_response(frame, sizeof(frame), 0x0057, &v) == KMP_ERR_CRC);
    frame[8] ^= 0x01;

    CHECK(kmp_parse_response(frame, sizeof(frame) - 1, 0x0057, &v) == KMP_ERR_FRAME);
    CHECK(kmp_parse_response(frame, 1, 0x0057, &v) == KMP_ERR_FRAME);

    const uint8_t dangling_esc[] = {0x3F, 0x10, 0x1B, 0x0D};
    CHECK(kmp_parse_response(dangling_esc, sizeof(dangling_esc), 0x0057, &v) == KMP_ERR_FRAME);
}

int main(void) {
    test_crc();
    test_request();
    test_response_stuffed();
    test_response_plain_and_negative();
    test_errors();
    printf(failures ? "%d FAILURE(S)\n" : "all KMP codec tests passed\n", failures);
    return failures != 0;
}
