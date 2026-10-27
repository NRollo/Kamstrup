# KMPLog - Kamstrup Multical 403 to MQTT

ESP32-C3 (LOLIN C3 Mini) firmware that reads a Kamstrup Multical 403 through its optical
interface using the Kamstrup Meter Protocol (KMP) and publishes the values to MQTT.

## Wiring

| ESP32-C3 | Optical head |
|----------|--------------|
| GPIO1    | RX (from head) |
| GPIO3    | TX (to head)   |

UART1, 1200 baud, 8 data bits, no parity, 2 stop bits.

The meter's optical interface only answers after the button on the meter has been
pressed, and stays awake for a few minutes. If the log shows `No answer from meter`,
press the button.

## Configuration

Copy `include/secrets.example.h` to `include/secrets.h` (git-ignored) and fill in the
WiFi and MQTT settings. Pins, read interval (10 s) and base topic are `#define`s at the
top of `src/main.c`.

## MQTT

| Topic | Payload |
|-------|---------|
| `kamstrup/status` | `online` / `offline` (retained, offline is the last will) |
| `kamstrup/state`  | JSON with the latest readings |

```json
{"heat_energy":868.48,"heat_energy_unit":"GJ","power":3.2,"power_unit":"kW",
 "temp1":71.5,"temp1_unit":"C","temp2":41.0,"temp2_unit":"C","temp_diff":30.5,
 "temp_diff_unit":"K","flow":410,"flow_unit":"l/h","volume":5321.7,"volume_unit":"m3"}
```

Units come from the meter, so heat energy appears in whatever unit the meter is
configured for (GJ, kWh, ...). Only registers that answered are included.

## KMP registers read

| Name | Register |
|------|----------|
| heat_energy | 0x003C |
| power | 0x0050 |
| temp1 | 0x0056 |
| temp2 | 0x0057 |
| temp_diff | 0x0059 |
| flow | 0x004A |
| volume | 0x0044 |

## Layout

- `src/kmp_codec.c`: framing, CRC-16/XMODEM, byte stuffing and response decoding (no ESP-IDF dependency)
- `src/kmp.c`: UART transport and polling
- `src/meter_mqtt.c`: MQTT client and JSON publishing
- `src/wifi_manager.c`: WiFi station
- `src/idf_component.yml`: managed components (`espressif/mqtt`, `espressif/cjson`, `joltwallet/littlefs`)

## Build

On Windows use `BUILD_WINDOWS.bat` or the VS Code build task (see `WINDOWS_BUILD.md`).

## Tests

The codec is tested on the host:

```
gcc -Wall -Werror -Iinclude test/host/kmp_codec_test.c src/kmp_codec.c -lm -o kmp_codec_test
kmp_codec_test
```
