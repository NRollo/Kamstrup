#ifndef METER_MQTT_H
#define METER_MQTT_H

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "kmp.h"

/**
 * @file meter_mqtt.h
 * @brief Publishes meter readings to an MQTT broker.
 */

/** @brief MQTT connection settings. All strings must stay valid while the client runs. */
typedef struct {
    const char *uri;        /**< Broker URI, e.g. "mqtt://192.168.0.113:1883". */
    const char *username;   /**< User name, NULL or empty for anonymous. */
    const char *password;   /**< Password, used only if a user name is given. */
    const char *client_id;  /**< Client ID; must be unique on the broker. */
    const char *base_topic; /**< Readings go to `<base>/state`, availability to `<base>/status`. */
} meter_mqtt_config_t;

/**
 * @brief Starts the MQTT client, which connects and reconnects in the background.
 *
 * Registers a retained last will "offline" on `<base>/status` and publishes a retained "online"
 * there after each (re)connect.
 *
 * @param cfg Connection settings.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG for missing settings,
 *         ESP_ERR_INVALID_SIZE if a topic is too long, otherwise an MQTT client error.
 */
esp_err_t meter_mqtt_start(const meter_mqtt_config_t *cfg);

/**
 * @brief Tells whether the client is currently connected to the broker.
 *
 * @return true if connected.
 */
bool meter_mqtt_is_connected(void);

/**
 * @brief Publishes the valid readings as one JSON object on `<base>/state`.
 *
 * Each reading adds a value and a unit, e.g. `{"power":3.2,"power_unit":"kW",...}`.
 *
 * @param readings Readings to publish; entries that are not valid are skipped.
 * @param count    Number of entries in @p readings.
 * @return ESP_OK if published, ESP_ERR_INVALID_STATE if not connected,
 *         ESP_ERR_NOT_FOUND if there was no valid reading, ESP_ERR_NO_MEM or ESP_FAIL on failure.
 */
esp_err_t meter_mqtt_publish_readings(const kmp_reading_t *readings, size_t count);

#endif
