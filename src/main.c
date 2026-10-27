#include "driver/uart.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "kmp.h"
#include "meter_mqtt.h"
#include "secrets.h"
#include "wifi_manager.h"

static const char *TAG = "MAIN";

#define KMP_UART_PORT     UART_NUM_1
#define KMP_RX_PIN        1
#define KMP_TX_PIN        3
#define READ_INTERVAL_SEC 10
#define MQTT_BASE_TOPIC   "kamstrup"

#define STR_(x) #x
#define STR(x)  STR_(x)

static void meter_task(void *arg) {
    kmp_reading_t readings[KMP_NUM_VALUES];

    while (1) {
        int valid = kmp_poll(readings);
        if (valid > 0) {
            esp_err_t err = meter_mqtt_publish_readings(readings, KMP_NUM_VALUES);
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "Publish failed: %s", esp_err_to_name(err));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_SEC * 1000));
    }
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(kmp_init(KMP_UART_PORT, KMP_TX_PIN, KMP_RX_PIN));

    wifi_init(WIFI_SSID, WIFI_PASSWORD);

    const meter_mqtt_config_t mqtt_cfg = {
        .uri = "mqtt://" MQTT_BROKER ":" STR(MQTT_PORT),
        .username = MQTT_USERNAME,
        .password = MQTT_PASSWORD,
        .client_id = "kamstrup_logger",
        .base_topic = MQTT_BASE_TOPIC,
    };
    ESP_ERROR_CHECK(meter_mqtt_start(&mqtt_cfg));

    xTaskCreate(meter_task, "meter", 6144, NULL, 5, NULL);
}
