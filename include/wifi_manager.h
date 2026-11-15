#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>

/**
 * @file wifi_manager.h
 * @brief WiFi station setup with automatic reconnect.
 */

/**
 * @brief Initializes WiFi in station mode and waits up to 20 s for an IP address.
 *
 * If the connection is not established in time, the function returns and keeps retrying
 * in the background. Requires the default event loop and NVS to be initialized.
 *
 * @param ssid     Network name.
 * @param password WPA2 password.
 * @return true if connected within the timeout, false otherwise.
 */
bool wifi_init(const char *ssid, const char *password);

/**
 * @brief Tells whether the station currently has an IP address.
 *
 * @return true if connected.
 */
bool wifi_is_connected(void);

/**
 * @brief Gets the current IP address as a string.
 *
 * @return IP address such as "192.168.0.50", or "not connected".
 */
const char* wifi_get_ip_address(void);

#endif
