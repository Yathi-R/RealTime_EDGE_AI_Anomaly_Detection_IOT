/*
 * ecg_driver_deep.c
 *
 *  Created on: Jul 30, 2025
 *      Author: Yathi
 */


#include "main.h"
#include "FreeRTOS.h"
#include "ai_datatypes_defines.h"
#include "ecgai_network.h"
#include "ecgai_network_data.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define MODEL_INPUT_SIZE     1000
#define AI_INPUT_SCALE       (0.028994000f)
#define AI_INPUT_ZERO_POINT  (114)
#define AI_OUTPUT_SCALE      (0.00390625f)
#define AI_OUTPUT_ZERO_POINT (0)
#define NORMALIZATION_FACTOR (0.5f)  // From model training

#define __CLAMP(x, min, max)  ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

/* ADC and UART handles */
extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef huart3;

/* Semaphores */
SemaphoreHandle_t adcSemaphore;
SemaphoreHandle_t uartSemaphore;

/* ECG data buffers */
static float modelInputBuffer[MODEL_INPUT_SIZE];
static uint32_t sampleCount = 0;
static float dynamicMean = 0.0f;
static float dynamicStd = 1.0f;  // Initialize with safe value

AI_ALIGNED(4) static ai_u8 ai_activations[AI_ECGAI_NETWORK_DATA_ACTIVATIONS_SIZE];

/* AI network and buffers */
static ai_handle ecg_network = AI_HANDLE_NULL;
AI_ALIGNED(4) static ai_u8 ai_input_buffer[AI_ECGAI_NETWORK_IN_1_SIZE_BYTES];
AI_ALIGNED(4) static ai_u8 ai_output_buffer[AI_ECGAI_NETWORK_OUT_1_SIZE_BYTES];

/* Network IO buffers */
static ai_buffer* ai_input = NULL;
static ai_buffer* ai_output = NULL;

/* Extern handles */
extern ai_handle g_ecgai_network_weights_table[];
extern ai_handle g_ecgai_network_activations_table[];

/* Statistics functions */
static float computeMean(float *buffer, uint32_t size) {
    float sum = 0.0f;
    for (uint32_t i = 0; i < size; i++) sum += buffer[i];
    return sum / size;
}

static float computeStd(float *buffer, uint32_t size, float mean) {
    float variance = 0.0f;
    for (uint32_t i = 0; i < size; i++) {
        float diff = buffer[i] - mean;
        variance += diff * diff;
    }
    return sqrtf(variance / size);
}

void MonitorHeap(void) {
    static TickType_t last = 0;
    char buf[128];
    if (xTaskGetTickCount() - last > pdMS_TO_TICKS(5000)) {
        last = xTaskGetTickCount();
        snprintf(buf, sizeof(buf), "Free heap: %u bytes\n", xPortGetFreeHeapSize());

        HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
    }
}

/* Initialize AI network */
static void InitAI(void) {

	g_ecgai_network_activations_table[0] = (ai_handle)ai_activations;

    ai_error err = ai_ecgai_network_create_and_init(
        &ecg_network,
        g_ecgai_network_activations_table,
        g_ecgai_network_weights_table
    );

    if (err.type != AI_ERROR_NONE) {
        char buf[64];
        snprintf(buf, sizeof(buf), "AI Init error: type=%d, code=%d\r\n", err.type, err.code);
        if (xSemaphoreTake(uartSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
            HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
            xSemaphoreGive(uartSemaphore);
        }
        ecg_network = AI_HANDLE_NULL;
    } else {
        // Get network IO buffers using generated functions
        ai_input = ai_ecgai_network_inputs_get(ecg_network, NULL);
        ai_output = ai_ecgai_network_outputs_get(ecg_network, NULL);

        // Assign data buffers
        if(ai_input && ai_output) {
        	ai_input[0].data = AI_HANDLE_PTR(ai_input_buffer);
            ai_output[0].data = AI_HANDLE_PTR(ai_output_buffer);
        }
    }
}

/* Read ADC value */
static inline int16_t ReadADC(void) {
    if (xSemaphoreTake(adcSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
            int16_t value = HAL_ADC_GetValue(&hadc1);
            HAL_ADC_Stop(&hadc1);
            xSemaphoreGive(adcSemaphore);
            return (value > 0) ? value : 2048;
        }
        HAL_ADC_Stop(&hadc1);
        xSemaphoreGive(adcSemaphore);
    }
    return 2048; // Return mid-scale if ADC read fails
}

/* Main ECG sampling & inference task */
void ECGSamplingTask(void *argument) {
    InitAI();

    if(ecg_network == AI_HANDLE_NULL || ai_input == NULL || ai_output == NULL) {
            HAL_UART_Transmit(&huart3, (uint8_t*)"AI Init Failed!\r\n", 16, 100);
            vTaskDelete(NULL);
    }

    TickType_t last_wake = xTaskGetTickCount();
    char uartBuffer[128];
    MonitorHeap();

    while (1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(1));

        /* Acquire sample and center at 0V */
        int16_t raw = ReadADC();
        float voltage = raw * (3.3f / 4095.0f);
        float centered = __CLAMP((voltage - 1.65f), -1.65f, 1.65f);  // Vref/2 = 1.65V
//        snprintf(uartBuffer, sizeof(uartBuffer), "ADC Test: raw=%d, voltage=%.3fV\r\n", raw, voltage);
//        HAL_UART_Transmit(&huart3, (uint8_t*)uartBuffer, strlen(uartBuffer), 100);

        /* Store in buffer */
        if (sampleCount < MODEL_INPUT_SIZE) {
            modelInputBuffer[sampleCount++] = centered;
        }

        /* Process when buffer full */
        if (sampleCount >= MODEL_INPUT_SIZE) {
            /* Compute statistics */
            dynamicMean = computeMean(modelInputBuffer, MODEL_INPUT_SIZE);
            dynamicStd = computeStd(modelInputBuffer, MODEL_INPUT_SIZE, dynamicMean);

            /* Normalize and quantize */
        	uint8_t min_val = 255, max_val = 0;
            for (int i = 0; i < MODEL_INPUT_SIZE; i++) {
                /* Match training normalization: (x - mean) / (std + epsilon) */
                float normalized = (modelInputBuffer[i] - dynamicMean) / (dynamicStd + 1e-8f);
//            	float normalized = (modelInputBuffer[i] / 1.65f) / NORMALIZATION_FACTOR; // Scaled to [-1,1]
                /* Apply training scaling factor */
                normalized /= NORMALIZATION_FACTOR;

                /* Quantize */
                int32_t q = (int32_t)roundf(normalized / AI_INPUT_SCALE) + AI_INPUT_ZERO_POINT;
                ai_input_buffer[i] = (ai_u8)__CLAMP(q, 0, 255);

            /* Log input statistics */

//            for (int i = 0; i < MODEL_INPUT_SIZE; i++) {
                if (ai_input_buffer[i] < min_val) min_val = ai_input_buffer[i];
                if (ai_input_buffer[i] > max_val) max_val = ai_input_buffer[i];
            }

                /* Validate Input Range */
             if(min_val < 0 || max_val > 255) { // Allow some margin
                      HAL_UART_Transmit(&huart3, (uint8_t*)"Quant Error!\r\n", 13, 100);
                      sampleCount = 0;
                      continue;
             }
            snprintf(uartBuffer, sizeof(uartBuffer),
                     "Input stats: min=%u, max=%u, mean=%.4f, std=%.4f\r\n",
                     min_val, max_val, dynamicMean, dynamicStd);
            if (xSemaphoreTake(uartSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
                HAL_UART_Transmit(&huart3, (uint8_t*)uartBuffer, strlen(uartBuffer), HAL_MAX_DELAY);
                xSemaphoreGive(uartSemaphore);
            }

            /* Run inference if network initialized */

            ai_i32 ret = ai_ecgai_network_run(ecg_network, ai_input, ai_output);

                if (ret <= 0) {
                    ai_error err = ai_ecgai_network_get_error(ecg_network);
                    snprintf(uartBuffer, sizeof(uartBuffer),
                             "AI run failed: ret=%ld, type=%d, code=%d\r\n",
                             ret, err.type, err.code);

                     HAL_UART_Transmit(&huart3, (uint8_t*)uartBuffer, strlen(uartBuffer), HAL_MAX_DELAY);
                } else {
                    ai_u8 yq = ai_output_buffer[0];
                    float score = ((float)yq - AI_OUTPUT_ZERO_POINT) * AI_OUTPUT_SCALE;
                    int is_anomaly = score > 0.5f ? 1 : 0;
                    snprintf(uartBuffer, sizeof(uartBuffer),
                             "Anomaly:%d,Score:%.2f\r\n", is_anomaly, score);

                        HAL_UART_Transmit(&huart3, (uint8_t*)uartBuffer, strlen(uartBuffer), HAL_MAX_DELAY);

                }
                /* Reset buffer */
                sampleCount = 0;
            }


            /* Debug logging every 100 samples */
                    if (sampleCount % 100 == 0 && sampleCount > 0) {
                        snprintf(uartBuffer, sizeof(uartBuffer),
                                 "Sample:%lu, Raw:%d, Centered:%.4f\r\n",
                                 sampleCount, ReadADC(), centered);

                            HAL_UART_Transmit(&huart3, (uint8_t*)uartBuffer, strlen(uartBuffer), HAL_MAX_DELAY);
        }
    }
}

/* ECG Task and Semaphore Initialization */
void ECGTask_Init(void) {
    adcSemaphore = xSemaphoreCreateMutex();
    uartSemaphore = xSemaphoreCreateMutex();
}



/* Stack Overflow Hook */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    char buf[128];
    snprintf(buf, sizeof(buf), "Stack overflow in %s\r\n", pcTaskName);
    if (xSemaphoreTake(uartSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
        HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
        xSemaphoreGive(uartSemaphore);
    }
    taskDISABLE_INTERRUPTS();
    while (1);
}
