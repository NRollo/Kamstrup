#include "kmp.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

#define KMP_BAUD_RATE        1200
#define KMP_RX_BUFFER_SIZE   256
#define KMP_RESPONSE_TIMEOUT_MS 1000

// Set to 1 to log every received byte, including the echo of our own request (receiver hardware test)
#define KMP_RX_DEBUG         1

static const char *TAG = "KMP";
static uart_port_t s_port = UART_NUM_1;

static const struct {
    uint16_t reg;
    const char *name;
} REGISTERS[KMP_NUM_VALUES] = {
    {0x003C, "heat_energy"},
    {0x0050, "power"},
    {0x0056, "temp1"},
    {0x0057, "temp2"},
    {0x0059, "temp_diff"},
    {0x004A, "flow"},
    {0x0044, "volume"},
};

esp_err_t kmp_init(uart_port_t port, int tx_pin, int rx_pin) {
    // The optical interface is 1200 baud, 8 data bits, no parity, 2 stop bits
    const uart_config_t cfg = {
        .baud_rate = KMP_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_2,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    s_port = port;
    ESP_RETURN_ON_ERROR(uart_driver_install(port, KMP_RX_BUFFER_SIZE, 0, 0, NULL, 0), TAG, "uart_driver_install");
    ESP_RETURN_ON_ERROR(uart_param_config(port, &cfg), TAG, "uart_param_config");
    ESP_RETURN_ON_ERROR(uart_set_pin(port, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE), TAG,
                        "uart_set_pin");
    // TX drives the IR LED through an NPN (pin high = LED on), so the idle level must be inverted.
    // The RX phototransistor already has the right polarity (light = low).
    ESP_RETURN_ON_ERROR(uart_set_line_inverse(port, UART_SIGNAL_TXD_INV), TAG, "uart_set_line_inverse");
    ESP_LOGI(TAG, "UART%d ready: TX=GPIO%d RX=GPIO%d, %d baud 8N2", port, tx_pin, rx_pin, KMP_BAUD_RATE);
    return ESP_OK;
}

/*
 * Collects one response frame. Anything before the 0x40 start byte (such as an
 * echo of our own request from the optical head) is ignored.
 */
static esp_err_t read_frame(uint8_t *frame, size_t max, size_t *len) {
    const int64_t deadline = esp_timer_get_time() + (int64_t)KMP_RESPONSE_TIMEOUT_MS * 1000;
    bool started = false;
    size_t n = 0;

    while (esp_timer_get_time() < deadline) {
        uint8_t b;
        if (uart_read_bytes(s_port, &b, 1, pdMS_TO_TICKS(20)) != 1) {
            continue;
        }
#if KMP_RX_DEBUG
        ESP_LOGI(TAG, "RX 0x%02X", b);
#endif
        if (b == KMP_RESP_START) {
            started = true;
            n = 0;
            continue;
        }
        if (!started) {
            continue;
        }
        if (n >= max) {
            return ESP_ERR_INVALID_RESPONSE;
        }
        frame[n++] = b;
        if (b == KMP_EOM) {
            *len = n;
            return ESP_OK;
        }
    }
    return started ? ESP_ERR_INVALID_RESPONSE : ESP_ERR_TIMEOUT;
}

esp_err_t kmp_read_register(uint16_t reg, kmp_value_t *out) {
    uint8_t request[KMP_MAX_REQUEST_LEN];
    uint8_t frame[KMP_MAX_FRAME_LEN];
    size_t frame_len = 0;

    size_t req_len = kmp_build_request(reg, request, sizeof(request));
    if (req_len == 0) {
        return ESP_ERR_INVALID_SIZE;
    }

    uart_flush_input(s_port);
    uart_write_bytes(s_port, request, req_len);
    uart_wait_tx_done(s_port, pdMS_TO_TICKS(500));
    //ESP_LOGHEX(TAG, "TX", request, req_len);
    esp_err_t err = read_frame(frame, sizeof(frame), &frame_len);
    if (err != ESP_OK) {
        return err;
    }

    kmp_status_t st = kmp_parse_response(frame, frame_len, reg, out);
    if (st != KMP_OK) {
        ESP_LOGW(TAG, "Register 0x%04X: bad response (status %d)", reg, st);
        return ESP_ERR_INVALID_RESPONSE;
    }
    return ESP_OK;
}

int kmp_poll(kmp_reading_t readings[KMP_NUM_VALUES]) {
    int valid = 0;

    for (int i = 0; i < KMP_NUM_VALUES; i++) {
        readings[i].name = REGISTERS[i].name;
        readings[i].valid = false;

        esp_err_t err = kmp_read_register(REGISTERS[i].reg, &readings[i].value);
        if (err == ESP_OK) {
            readings[i].valid = true;
            valid++;
            ESP_LOGD(TAG, "%s = %g %s", readings[i].name, readings[i].value.value,
                     kmp_unit_name(readings[i].value.unit));
        } else if (err == ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "No answer from meter - is the optical interface awake? (press the meter button)");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay 100ms between register reads
    }
    return valid;
}
