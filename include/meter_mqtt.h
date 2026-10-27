#ifndef METER_MQTT_H
#define METER_MQTT_H

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "kmp.h"

typedef struct {
    const char *uri;        // e.g. "mqtt://192.168.0.113:1883"
    const char *username;   // NULL or empty for anonymous
    const char *password;
    const char *client_id;
    const char *base_topic; // state goes to <base>/state, availability to <base>/status
} meter_mqtt_config_t;

esp_err_t meter_mqtt_start(const meter_mqtt_config_t *cfg);
bool meter_mqtt_is_connected(void);

/* Publishes the valid readings as one JSON object, e.g. {"power":3.2,"power_unit":"kW",...}. */
esp_err_t meter_mqtt_publish_readings(const kmp_reading_t *readings, size_t count);

#endif
