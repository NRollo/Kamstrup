#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>

// Initialize WiFi and connect to network
bool wifi_init(const char *ssid, const char *password);

// Check if WiFi is connected
bool wifi_is_connected(void);

// Get the current IP address as string
const char* wifi_get_ip_address(void);

#endif
