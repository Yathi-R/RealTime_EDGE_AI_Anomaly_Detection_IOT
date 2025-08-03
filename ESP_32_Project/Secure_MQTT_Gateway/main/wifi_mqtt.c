/*
 * wifi_mqtt.c
 * Simplified Wi-Fi and MQTT initialization
 */

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_wifi_types_generic.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "wifi_mqtt.h"


static const char *TAG = "wifi_mqtt";

bool wifi_connected = false;
esp_mqtt_client_handle_t mqtt_client = NULL;


// Global station config so it can be referenced anywhere
static wifi_config_t sta_cfg = {
    .sta = {
        .ssid = "yathi_desd",            // <-- set your SSID
        .password = "cdacdesd",    // <-- set your Password
        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        .pmf_cfg = {
            .capable = true,
            .required = false,
        },
    },
};

// MQTT event handler to drain buffer on connect
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    if (event_id == MQTT_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "MQTT connected:");
        }
    else if (event_id == MQTT_EVENT_DISCONNECTED) {
        ESP_LOGW(TAG, "MQTT disconnected");
    }
}


static void wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_START, connecting...");
        esp_wifi_connect();
    } 
    else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* evt = data;
        ESP_LOGW(TAG, "STA_DISCONNECTED, reason: %d", evt->reason);
        wifi_connected = false;
        esp_wifi_connect();
    } 
    else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        wifi_connected = true;
        ip_event_got_ip_t* evt = data;
        ESP_LOGI(TAG, "GOT IP: " IPSTR, IP2STR(&evt->ip_info.ip));
        
        if (mqtt_client) {
            esp_mqtt_client_start(mqtt_client);
        }
    }
}

void wifi_mqtt_init(void)
{
    // Initialize NVS — required for Wi-Fi
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));

    // Register our event handler for Wi-Fi and IP events
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                               &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               &wifi_event_handler, NULL));

    // Set Wi-Fi mode and config
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));
    ESP_LOGI(TAG, "Connecting to SSID: %s", sta_cfg.sta.ssid);
    //ESP_LOGI(TAG, "Using password: %s", (char*)sta_cfg.sta.password);
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    
    ESP_ERROR_CHECK(esp_wifi_start());

    // Configure MQTT
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtts://192.168.137.1:8883", // broker IP
        
        //.broker.verification.use_global_ca_store   = true,
        .broker.verification.crt_bundle_attach = esp_crt_bundle_attach,
        //.broker.verification.certificate = NULL,
		//.broker.verification.certificate_len = 0,
		//.broker.verification.skip_cert_common_name_check = false,

        .credentials = {
            .username = "cdacdesd",
            .client_id = "esp32_client",
            .authentication.password = "RTAI_IOT",
        },
    };
    
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    ESP_ERROR_CHECK(mqtt_client ? ESP_OK : ESP_FAIL);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                   mqtt_event_handler, NULL);
    
    ESP_LOGI(TAG, "Wi-Fi and MQTT initialization done");
}





















