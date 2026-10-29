#include "status_led.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi_manager.h"

#define STATUS_LED_PIN         8
// The on-board LED of the ESP32-C3 SuperMini is wired between 3V3 and GPIO8, so it lights when the pin is low
#define STATUS_LED_ACTIVE_LOW  1
#define FAST_BLINK_MS          100
#define PULSE_MS               100

static volatile bool s_pulse_pending;

static void led_set(bool on) {
    gpio_set_level(STATUS_LED_PIN, on ^ STATUS_LED_ACTIVE_LOW);
}

static void led_task(void *arg) {
    bool level = false;

    while (1) {
        if (!wifi_is_connected()) {
            level = !level;
            led_set(level);
            vTaskDelay(pdMS_TO_TICKS(FAST_BLINK_MS));
        } else if (s_pulse_pending) {
            s_pulse_pending = false;
            led_set(true);
            vTaskDelay(pdMS_TO_TICKS(PULSE_MS));
            led_set(false);
            vTaskDelay(pdMS_TO_TICKS(PULSE_MS));
        } else {
            led_set(false);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}

void status_led_start(void) {
    gpio_reset_pin(STATUS_LED_PIN);
    gpio_set_direction(STATUS_LED_PIN, GPIO_MODE_OUTPUT);
    led_set(false);
    xTaskCreate(led_task, "status_led", 2048, NULL, 3, NULL);
}

void status_led_pulse(void) {
    s_pulse_pending = true;
}
