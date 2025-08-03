
/*
 * main.c
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "wifi_mqtt.h"
#include "uart_ecg.h"
#include "sdkconfig.h"
#include "esp_wifi.h"
#include "miniz.h"

#define BLINK_GPIO     			CONFIG_BLINK_GPIO
#define BLINK_PERIOD   			3000
#define LINE_BUFFER_SIZE       	30    // Max JSON line length
#define MQTT_PUB_TOPIC         	"ecg/data"

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

static const char *TAG_MAIN = "Main";
static uint8_t s_led_state = 0;

static uint8_t s_line_buffer[LINE_BUFFER_SIZE];
static size_t s_line_len = 0;
static bool s_skipping = false;

void ecg_task(void *pvParam) {
	
	
    ESP_LOGI(TAG_MAIN, "ECG task started");

    while (1) {
        if (!wifi_connected) {
            // Attempt WiFi reconnection
            if (!esp_wifi_connect()) {
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
        }

        // Check for available UART data
        size_t available = 0;
        ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_PORT, &available));
        
        if (available == 0) {
            vTaskDelay(10); // Yield CPU
            continue;
        }

        // Read available data
		uint8_t data[128];  // Optimized read chunk size
        int read = uart_read_bytes(UART_PORT, data, MIN(available, sizeof(data)), 10);
        if (read <= 0) continue;


        // Process received bytes
        for (int i = 0; i < read; i++) {
            uint8_t c = data[i];
            
            if (c == '\n') {
                if (!s_skipping && s_line_len > 0) {
                    // Publish complete line
                    esp_mqtt_client_publish(mqtt_client, MQTT_PUB_TOPIC, (char*)s_line_buffer, s_line_len, 1, 0);
                    gpio_set_level(BLINK_GPIO, s_led_state);
        			s_led_state ^= 1;
        			
                    s_line_len = 0;
                }
                s_skipping = false;
            } else {
                if (!s_skipping) {
                    if (s_line_len < LINE_BUFFER_SIZE - 1) {
                        s_line_buffer[s_line_len++] = c;
                    } else {
                        // Handle buffer overflow
                        if (s_line_len > 0) {
                            esp_mqtt_client_publish(mqtt_client, MQTT_PUB_TOPIC, (char*)s_line_buffer, s_line_len, 1, 0);
                            gpio_set_level(BLINK_GPIO, s_led_state);
        					s_led_state ^= 1;
                        }
                        s_line_len = 0;
                        s_skipping = true; // Skip until next newline
                    }
                }
            }
        }
        vTaskDelay(100); // Yield to other tasks
    }
}





void app_main(void)
{
    // Initialize peripherals
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

   // buffer_init();
    uart_ecg_init();

    // Initialize network and MQTT
    wifi_mqtt_init();

    // Launch ECG processing task
    xTaskCreate(ecg_task, "ecg_task", 4096, NULL, 10, NULL);


    while (1) {
        //ESP_LOGI(TAG_MAIN, "LED %s", s_led_state ? "ON" : "OFF");
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}









