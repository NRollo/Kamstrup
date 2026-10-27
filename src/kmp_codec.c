#include "kmp_codec.h"
#include <math.h>

static const char *const UNIT_NAMES[] = {
    "", "Wh", "kWh", "MWh", "GWh", "J", "kJ", "MJ", "GJ", "Cal", "kCal", "Mcal", "Gcal",
    "varh", "kvarh", "Mvarh", "Gvarh", "VAh", "kVAh", "MVAh", "GVAh", "kW", "kW", "MW", "GW",
    "kvar", "kvar", "Mvar", "Gvar", "VA", "kVA", "MVA", "GVA", "V", "A", "kV", "kA", "C", "K",
    "l", "m3", "l/h", "m3/h", "m3xC", "ton", "ton/h", "h", "hh:mm:ss", "yy:mm:dd", "yyyy:mm:dd",
    "mm:dd", "", "bar", "RTC", "ASCII", "m3 x 10", "ton x 10", "GJ x 10", "minutes", "Bitfield",
    "s", "ms", "days", "RTC-Q", "Datetime",
};

const char *kmp_unit_name(uint8_t unit) {
    if (unit >= sizeof(UNIT_NAMES) / sizeof(UNIT_NAMES[0])) {
        return "";
    }
    return UNIT_NAMES[unit];
}

uint16_t kmp_crc16(const uint8_t *data, size_t len) {
    uint16_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

static int needs_stuffing(uint8_t b) {
    return b == 0x06 || b == KMP_EOM || b == KMP_ESC || b == KMP_RESP_START || b == KMP_REQ_START;
}

size_t kmp_build_request(uint16_t reg, uint8_t *out, size_t out_size) {
    uint8_t msg[7] = {KMP_ADDR_HEAT_METER, KMP_CID_GET_REGISTER, 0x01, (uint8_t)(reg >> 8), (uint8_t)reg};
    uint16_t crc = kmp_crc16(msg, 5);
    msg[5] = (uint8_t)(crc >> 8);
    msg[6] = (uint8_t)crc;

    size_t n = 0;
    if (out_size < 1) {
        return 0;
    }
    out[n++] = KMP_REQ_START;
    for (size_t i = 0; i < sizeof(msg); i++) {
        if (needs_stuffing(msg[i])) {
            if (n + 2 > out_size) {
                return 0;
            }
            out[n++] = KMP_ESC;
            out[n++] = msg[i] ^ 0xFF;
        } else {
            if (n + 1 > out_size) {
                return 0;
            }
            out[n++] = msg[i];
        }
    }
    if (n + 1 > out_size) {
        return 0;
    }
    out[n++] = KMP_EOM;
    return n;
}

kmp_status_t kmp_parse_response(const uint8_t *frame, size_t len, uint16_t expected_reg, kmp_value_t *out) {
    uint8_t msg[KMP_MAX_FRAME_LEN];
    size_t n = 0;

    if (len < 2 || frame[len - 1] != KMP_EOM) {
        return KMP_ERR_FRAME;
    }
    for (size_t i = 0; i < len - 1; i++) {
        uint8_t b = frame[i];
        if (b == KMP_ESC) {
            if (i + 1 >= len - 1) {
                return KMP_ERR_FRAME;
            }
            b = frame[++i] ^ 0xFF;
        }
        if (n >= sizeof(msg)) {
            return KMP_ERR_FRAME;
        }
        msg[n++] = b;
    }

    if (n < 9) {
        return KMP_ERR_FRAME;
    }
    uint16_t rx_crc = (uint16_t)((msg[n - 2] << 8) | msg[n - 1]);
    if (kmp_crc16(msg, n - 2) != rx_crc) {
        return KMP_ERR_CRC;
    }
    if (msg[0] != KMP_ADDR_HEAT_METER || msg[1] != KMP_CID_GET_REGISTER) {
        return KMP_ERR_HEADER;
    }
    if ((uint16_t)((msg[2] << 8) | msg[3]) != expected_reg) {
        return KMP_ERR_REGISTER;
    }

    uint8_t unit = msg[4];
    uint8_t mantissa_len = msg[5];
    uint8_t sigexp = msg[6];
    if (mantissa_len > 4 || n != (size_t)(7 + mantissa_len + 2)) {
        return KMP_ERR_PAYLOAD;
    }

    uint32_t mantissa = 0;
    for (uint8_t i = 0; i < mantissa_len; i++) {
        mantissa = (mantissa << 8) | msg[7 + i];
    }
    int exponent = sigexp & 0x3F;
    if (sigexp & 0x40) {
        exponent = -exponent;
    }
    double value = (double)mantissa * pow(10.0, exponent);
    if (sigexp & 0x80) {
        value = -value;
    }

    out->reg = expected_reg;
    out->unit = unit;
    out->value = value;
    return KMP_OK;
}
