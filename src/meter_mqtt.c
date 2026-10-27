#include "meter_mqtt.h"
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "MQTT";

#define TOPIC_MAX 96

static esp_mqtt_client_handle_t s_client;
static volatile bool s_connected;
static char s_state_topic[TOPIC_MAX];
static char s_status_topic[TOPIC_MAX];

static void mqtt_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data) {
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Connected");
        s_connected = true;
        esp_mqtt_client_publish(s_client, s_status_topic, "online", 0, 1, true);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Disconnected");
        s_connected = false;
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGW(TAG, "Error event");
        break;
    default:
        break;
    }
}

esp_err_t meter_mqtt_start(const meter_mqtt_config_t *cfg) {
    if (!cfg || !cfg->uri || !cfg->base_topic) {
        return ESP_ERR_INVALID_ARG;
    }
    if (snprintf(s_state_topic, sizeof(s_state_topic), "%s/state", cfg->base_topic) >= TOPIC_MAX ||
        snprintf(s_status_topic, sizeof(s_status_topic), "%s/status", cfg->base_topic) >= TOPIC_MAX) {
        return ESP_ERR_INVALID_SIZE;
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = cfg->uri,
        .credentials.client_id = cfg->client_id,
        .session.last_will = {
            .topic = s_status_topic,
            .msg = "offline",
            .msg_len = 0,
            .qos = 1,
            .retain = 1,
        },
    };
    if (cfg->username && cfg->username[0]) {
        mqtt_cfg.credentials.username = cfg->username;
        mqtt_cfg.credentials.authentication.password = cfg->password;
    }

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!s_client) {
        return ESP_FAIL;
    }
    esp_err_t err = esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if (err == ESP_OK) {
        err = esp_mqtt_client_start(s_client);
    }
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Connecting to %s, state topic %s", cfg->uri, s_state_topic);
    }
    return err;
}

bool meter_mqtt_is_connected(void) {
    return s_connected;
}

esp_err_t meter_mqtt_publish_readings(const kmp_reading_t *readings, size_t count) {
    if (!s_client || !s_connected) {
        return ESP_ERR_INVALID_STATE;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return ESP_ERR_NO_MEM;
    }

    size_t added = 0;
    for (size_t i = 0; i < count; i++) {
        if (!readings[i].valid) {
            continue;
        }
        char unit_key[32];
        snprintf(unit_key, sizeof(unit_key), "%s_unit", readings[i].name);
        cJSON_AddNumberToObject(root, readings[i].name, readings[i].value.value);
        cJSON_AddStringToObject(root, unit_key, kmp_unit_name(readings[i].value.unit));
        added++;
    }

    esp_err_t err = ESP_ERR_NOT_FOUND;
    if (added > 0) {
        char *json = cJSON_PrintUnformatted(root);
        if (!json) {
            err = ESP_ERR_NO_MEM;
        } else {
            int id = esp_mqtt_client_publish(s_client, s_state_topic, json, 0, 0, false);
            err = id >= 0 ? ESP_OK : ESP_FAIL;
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "%s -> %s", s_state_topic, json);
            }
            cJSON_free(json);
        }
    }
    cJSON_Delete(root);
    return err;
}
