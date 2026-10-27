#ifndef KMP_H
#define KMP_H

#include <stdbool.h>
#include "driver/uart.h"
#include "esp_err.h"
#include "kmp_codec.h"

#define KMP_NUM_VALUES 7

typedef struct {
    const char *name;
    kmp_value_t value;
    bool valid;
} kmp_reading_t;

esp_err_t kmp_init(uart_port_t port, int tx_pin, int rx_pin);

/* Reads one register. ESP_ERR_TIMEOUT means the meter did not answer at all. */
esp_err_t kmp_read_register(uint16_t reg, kmp_value_t *out);

/*
 * Reads all standard registers into readings. Stops early if the meter is silent.
 * Returns the number of valid readings.
 */
int kmp_poll(kmp_reading_t readings[KMP_NUM_VALUES]);

#endif
