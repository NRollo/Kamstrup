#ifndef SECRETS_H
#define SECRETS_H

/**
 * @file secrets.example.h
 * @brief Template for the WiFi and MQTT credentials.
 *
 * Copy this file to secrets.h (git-ignored) and fill in your own values.
 */
#define WIFI_SSID     "your-ssid"          /**< WiFi network name. */
#define WIFI_PASSWORD "your-wifi-password" /**< WiFi password. */
#define MQTT_BROKER   "192.168.0.113"      /**< MQTT broker host name or IP address. */
#define MQTT_PORT     1883                 /**< MQTT broker TCP port. */
#define MQTT_USERNAME "your-mqtt-user"     /**< MQTT user name, empty for anonymous. */
#define MQTT_PASSWORD "your-mqtt-password" /**< MQTT password. */

#endif
