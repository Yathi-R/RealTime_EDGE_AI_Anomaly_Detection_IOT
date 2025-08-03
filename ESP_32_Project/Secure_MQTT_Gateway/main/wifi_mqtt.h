/*
 * wifi_mqtt.h
 * Simplified Wi-Fi + MQTT initializer
 */
#ifndef WIFI_MQTT_H
#define WIFI_MQTT_H

#include <stdbool.h>
#include "mqtt_client.h"

/** Initialize Wi-Fi STA and MQTT client.
 *  Call before starting application tasks.
 */
void wifi_mqtt_init(void);

/** Extern handles for use by other modules */
extern bool wifi_connected;
extern esp_mqtt_client_handle_t mqtt_client;

#endif // WIFI_MQTT_H