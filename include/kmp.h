#ifndef KMP_H
#define KMP_H

#include <stdbool.h>
#include "driver/uart.h"
#include "esp_err.h"
#include "kmp_codec.h"

/**
 * @file kmp.h
 * @brief Reads a Kamstrup heat meter over the optical interface using the KMP protocol on a UART.
 */

#define KMP_NUM_VALUES 7 /**< Number of registers read by kmp_poll(). */

/** @brief One named reading from the meter. */
typedef struct {
    const char *name;   /**< Name used as JSON key, e.g. "heat_energy". */
    kmp_value_t value;  /**< Decoded value; only meaningful if @ref valid is true. */
    bool valid;         /**< True if the register was read successfully. */
} kmp_reading_t;

/**
 * @brief Installs and configures the UART used for the optical interface.
 *
 * Uses 1200 baud, 8 data bits, no parity, 2 stop bits. The TX line is inverted because TX drives
 * the IR LED through an NPN transistor.
 *
 * @param port   UART port to use.
 * @param tx_pin GPIO connected to the IR LED driver.
 * @param rx_pin GPIO connected to the IR phototransistor.
 * @return ESP_OK on success, otherwise the error from the failing UART call.
 */
esp_err_t kmp_init(uart_port_t port, int tx_pin, int rx_pin);

/**
 * @brief Reads one register from the meter.
 *
 * @param reg      Register number to read.
 * @param[out] out Receives the decoded value on success.
 * @return ESP_OK on success, ESP_ERR_TIMEOUT if the meter did not answer at all,
 *         ESP_ERR_INVALID_RESPONSE if the answer was incomplete or invalid,
 *         ESP_ERR_INVALID_SIZE if the request could not be built.
 */
esp_err_t kmp_read_register(uint16_t reg, kmp_value_t *out);

/**
 * @brief Reads all standard registers.
 *
 * Stops early if the meter is silent. Registers with an invalid answer are marked as not valid
 * and skipped.
 *
 * @param[out] readings Array that receives one entry per register.
 * @return Number of valid readings.
 */
int kmp_poll(kmp_reading_t readings[KMP_NUM_VALUES]);

#endif
