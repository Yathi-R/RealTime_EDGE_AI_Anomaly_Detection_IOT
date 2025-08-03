/*
 * uart_ecg.c
 *
 *  Created on: 1 Aug 2025
 *      Author: Yathi
 */


#include "driver/uart.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "soc/gpio_num.h"


// Configuration
#define UART_BAUD_RATE         921600
#define UART_RX_BUFFER_SIZE    4096    // DMA buffer size


static const char *TAG = "ECG_Task";

// UART Configuration
#define UART_PORT          UART_NUM_1
#define UART_RX_PIN        21
#define UART_TX_PIN        22


void uart_ecg_init(void) {
	
	// UART configuration
    uart_config_t uart_cfg = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_APB,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_cfg));
    
    // Set UART pins
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN, 
                                UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    // Install UART driver with DMA buffer
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_RX_BUFFER_SIZE, 
                                        0, 0, NULL, 0));	
	

    ESP_LOGI(TAG, "ECG UART initialized at %d baud", UART_BAUD_RATE);
}


