/*
 * ECG_Driver.c
 *
 *  Created on: Jul 29, 2025
 *      Author: Yathi
 */


#include "stm32h5xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ecgai_network.h>
#include <ecgai_network_data.h>

/* ADC and UART handles (initialized externally in main.c) */
extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef huart3;
extern TaskHandle_t ecg_handle;

/* Semaphores for resource protection */
SemaphoreHandle_t adcSemaphore;
SemaphoreHandle_t uartSemaphore;

/* Buffer for ECG data (circular buffer for processing) */
#define ECG_BUFFER_SIZE 256
static uint16_t ecgRawBuffer[ECG_BUFFER_SIZE];
static float ecgFilteredBuffer[ECG_BUFFER_SIZE];
static uint32_t bufferIndex = 0;

#define MODEL_INPUT_SIZE 1000
static float modelInputBuffer[MODEL_INPUT_SIZE];
static uint32_t sampleCount = 0;

/* Filter coefficients for a simple 50Hz notch filter and high-pass filter */
static const float notchFilterB[] = {1.0f, -1.4142f, 1.0f}; /* 50Hz notch filter */
static const float notchFilterA[] = {1.0f, -1.8478f, 0.9114f};
static const float highPassB[] = {1.0f, -1.0f};             /* High-pass filter */
static const float highPassA[] = {1.0f, -0.995f};
static float notchState[2] = {0.0f, 0.0f};
static float hpState[1] = {0.0f};

/* X-CUBE-AI setup */
static ai_handle ecg_network = AI_HANDLE_NULL;
static ai_buffer ai_input[AI_ECGAI_NETWORK_IN_NUM];
static ai_buffer ai_output[AI_ECGAI_NETWORK_OUT_NUM];
static int8_t ai_input_buffer[MODEL_INPUT_SIZE];
static int8_t ai_output_buffer[AI_ECGAI_NETWORK_OUT_1_SIZE];

/* Quantization constants from your TFLite model */
#define AI_INPUT_SCALE       (0.0289939995855093f)  /* input scale */
#define AI_INPUT_ZERO_POINT  (114)                   /* input zero‑point */
#define AI_OUTPUT_SCALE      (0.00390625f)          /* output scale */
#define AI_OUTPUT_ZERO_POINT (0)                     /* output zero‑point */


/* Task handle */
//TaskHandle_t ecgTaskHandle = NULL;

/* Apply IIR filter (Direct Form I) */
static float applyIIRFilter(float input, const float *b, const float *a, float *state, uint32_t order) {
    float output = b[0] * input + state[0];
    for (uint32_t i = 1; i <= order; i++) {
        state[i - 1] = b[i] * input - a[i] * output + (i < order ? state[i] : 0);
    }
    return output;
}


/* Initialize X-CUBE-AI */
static void InitAI(void) {
    ai_error err;
//    ai_network_params params = {
//            AI_NETWORK_PARAMS_INIT,
//            .map_weights = AI_STRUCT_INIT,
//            .map_activations = AI_STRUCT_INIT
//        };
//
//    err = ai_ecg_network_create(&ecg_network, AI_ECG_NETWORK_DATA_CONFIG);
    ai_handle activations = NULL; /* Use default activation buffer */
    ai_handle weights = NULL;     /* Use default weights buffer */
    err = ai_ecgai_network_create_and_init(&ecg_network, &activations, &weights);
    if (err.type != AI_ERROR_NONE) {
        char uartBuffer[128];
        snprintf(uartBuffer, sizeof(uartBuffer), "AI init failed: %d\r\n", err.code);
        if (xSemaphoreTake(uartSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
            HAL_UART_Transmit(&huart3, (uint8_t *)uartBuffer, strlen(uartBuffer), 10);
            xSemaphoreGive(uartSemaphore);
        }
        //Error_Handler();
    }
    /* Configure input buffer */
    ai_input[0].format = AI_ECGAI_NETWORK_IN_1_FORMAT;
    ai_input[0].data = AI_HANDLE_PTR(ai_input_buffer);
    ai_input[0].meta_info = NULL;
    /* Configure output buffer */
    ai_output[0].format = AI_ECGAI_NETWORK_OUT_1_FORMAT;
    ai_output[0].data = AI_HANDLE_PTR(ai_output_buffer);
    ai_output[0].meta_info = NULL;
}



/* ECG Sampling & Processing Task */
void ECGSamplingTask(void *pvParameters) {
    uint16_t adcValue;
    float filteredValue, scaledValue;
    char uartBuffer[128];
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t samplingPeriod = pdMS_TO_TICKS(1); // 1ms (1kHz)

    /* Initialize AI */
    InitAI();

    while (1) {
        vTaskDelayUntil(&lastWakeTime, samplingPeriod);

        /* Acquire ADC semaphore */
        if (xSemaphoreTake(adcSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (HAL_ADC_Start(&hadc1) == HAL_OK &&
                HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
                adcValue = HAL_ADC_GetValue(&hadc1);
                ecgRawBuffer[bufferIndex] = adcValue;

				/* Convert to voltage */
				float voltage = ((float)adcValue * 3.3f ) / 4096.0f;

				/* Apply filters */
				filteredValue = applyIIRFilter(voltage, notchFilterB, notchFilterA, notchState, 2);
				filteredValue = applyIIRFilter(filteredValue, highPassB, highPassA, hpState, 1);

				/* Scale to millivolts */
				scaledValue = filteredValue * 100.0f;
				ecgFilteredBuffer[bufferIndex] = scaledValue;

				modelInputBuffer[sampleCount] = filteredValue; // Store unscaled for model
				sampleCount++;


				/* Prepare string */
				snprintf(uartBuffer, sizeof(uartBuffer), "%d %.2f %.2f\r\n", adcValue, voltage, scaledValue);

				/* Send over UART */
				if (xSemaphoreTake(uartSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
					HAL_UART_Transmit(&huart3, (uint8_t *)uartBuffer, strlen(uartBuffer), 10);
					xSemaphoreGive(uartSemaphore);
				}
            }
            HAL_ADC_Stop(&hadc1); // Always stop
            xSemaphoreGive(adcSemaphore);
            } else {
                    continue; // Skip this sample if ADC is busy
            }

        /* Run inference every 1000 samples (1 second) */
        if (sampleCount >= MODEL_INPUT_SIZE) {
            /* Normalize input buffer (zero mean, unit variance) */
            float mean = 0.0f, std = 0.0f;
            for (uint32_t i = 0; i < MODEL_INPUT_SIZE; i++) {
                mean += modelInputBuffer[i];
            }
            mean /= MODEL_INPUT_SIZE;
            for (uint32_t i = 0; i < MODEL_INPUT_SIZE; i++) {
                std += (modelInputBuffer[i] - mean) * (modelInputBuffer[i] - mean);
            }
            std = sqrtf(std / MODEL_INPUT_SIZE + 1e-8);
            for (uint32_t i = 0; i < MODEL_INPUT_SIZE; i++) {
                float norm = (modelInputBuffer[i] - mean) / (std + 1e-8f);
                int32_t q = (int32_t)roundf(norm / AI_INPUT_SCALE) + AI_INPUT_ZERO_POINT;
                if (q > 127) q = 127;
                if (q < -128) q = -128;
                ai_input_buffer[i] = (int8_t)q;
            }

            /* Run inference */
            ai_i32 n_batches = ai_ecgai_network_run(ecg_network, ai_input, ai_output);
            if (n_batches < 0) {
                snprintf(uartBuffer, sizeof(uartBuffer), "Inference failed\r\n");
                if (xSemaphoreTake(uartSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
                    HAL_UART_Transmit(&huart3, (uint8_t *)uartBuffer, strlen(uartBuffer), 10);
                    xSemaphoreGive(uartSemaphore);
                }
            } else {
            	int8_t q_out = ai_output_buffer[0];
            	float anomaly_score = ((float)q_out - AI_OUTPUT_ZERO_POINT) * AI_OUTPUT_SCALE;
//                float anomaly_score = ai_output_buffer[0];
                int is_anomaly = anomaly_score > 0.5 ? 1 : 0;
                snprintf(uartBuffer, sizeof(uartBuffer), "Anomaly:%d,Score:%.2f\r\n", is_anomaly, anomaly_score);
                if (xSemaphoreTake(uartSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
                    HAL_UART_Transmit(&huart3, (uint8_t *)uartBuffer, strlen(uartBuffer), 10);
                    xSemaphoreGive(uartSemaphore);
                }
            }
            sampleCount = 0; // Reset buffer
        }

        /* Circular buffer index */
        bufferIndex = (bufferIndex + 1) % ECG_BUFFER_SIZE;
    }
}

/* ECG Task and Semaphore Initialization */
void ECGTask_Init(void) {
    adcSemaphore = xSemaphoreCreateMutex();
    uartSemaphore = xSemaphoreCreateMutex();

    if (adcSemaphore == NULL || uartSemaphore == NULL) {
//        Error_Handler();
    }

//    if (xTaskCreate(ECGSamplingTask, "ECGTask", configMINIMAL_STACK_SIZE + 256, NULL, tskIDLE_PRIORITY + 2, &ecgTaskHandle) != pdPASS) {
//        Error_Handler();
//    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    /* Print task name and enter infinite loop to halt execution */
    char uartBuffer[128];
    snprintf(uartBuffer, sizeof(uartBuffer), "Stack overflow in task: %s\r\n", pcTaskName);
    if (xSemaphoreTake(uartSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
        HAL_UART_Transmit(&huart3, (uint8_t *)uartBuffer, strlen(uartBuffer), 10);
        xSemaphoreGive(uartSemaphore);
    }
    while (1) {
        /* Halt execution */
    }
}

/* User-defined error handler */


