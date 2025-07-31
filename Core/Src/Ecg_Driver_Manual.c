/*
 * Ecg_Driver_Manual.c
 *
 *  Created on: Jul 31, 2025
 *      Author: Yathi
 */


#include "main.h"
#include "FreeRTOS.h"
#include "ai_datatypes_defines.h"
#include "network.h"
#include "network_data.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "ai_platform.h"
//#include "stai.h"

#define MODEL_INPUT_SIZE     AI_NETWORK_IN_1_SIZE

#define SAMPLE_RATE_MS       2  // 1ms = 1000Hz sampling

/* ADC and UART handles */
extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef huart3;

/* Semaphores */
SemaphoreHandle_t adcSemaphore;
SemaphoreHandle_t uartSemaphore;

/* ECG data buffers */
static float modelInputBuffer[MODEL_INPUT_SIZE];
static uint32_t sampleCount = 0;

/* AI network and buffers */

static ai_handle ecg_network = AI_HANDLE_NULL;

/* Input and output buffers */
AI_ALIGNED(4) static ai_float ai_input_buffer[MODEL_INPUT_SIZE];
AI_ALIGNED(4) static ai_float ai_output_buffer[AI_NETWORK_OUT_1_SIZE];
AI_ALIGNED(4) static ai_float ai_activations[AI_NETWORK_DATA_ACTIVATIONS_SIZE];

// After your ai_buffer declarations and before any functions:
static ai_buffer *in_bufs  = NULL;
static ai_buffer *out_bufs = NULL;


// Use generated weights and activations tables
extern ai_handle g_network_weights_table[];
extern ai_handle g_network_activations_table[];



/* AI buffer definitions */
//static ai_buffer ai_input[AI_NETWORK_IN_NUM] = {
//    AI_BUFFER_OBJ_INIT(
//    	STAI_FORMAT_FLOAT32,  // format_
//        1,						//h_
//		MODEL_INPUT_SIZE,		//w_
//        1,						//ch_
//        1,						//n_batches_
//		ai_input_buffer      // data_
//    )
//};



//static ai_buffer ai_output[AI_NETWORK_OUT_NUM] = {
//    AI_BUFFER_OBJ_INIT(
//    	AI_NETWORK_IN_1_FORMAT,  // format_
//        1,                    // h_ (height: 1)
//        1,                    // w_ (width: 1)
//        1,                    // ch_ (channels: 1)
//        1,                    // n_batches_ (batch size: 1)
//		NULL      // data_
//    )
//};

//static ai_buffer ai_input[AI_NETWORK_IN_NUM] = {
//  AI_BUFFER_INIT(
//    AI_FLAG_NONE,                        // flags
//    AI_NETWORK_IN_1_FORMAT,              // format
//    //AI_SHAPE_BCWH(                       // shape = [batch, channel, width, height]
////      1,                                 // batches
////      AI_NETWORK_IN_1_CHANNEL,           // channels
////      AI_NETWORK_IN_1_HEIGHT,            // width (1000)
////      1                                  // height (always 1 for a 1×1000 vector)
////    ),
//	AI_BUFFER_SHAPE_INIT(AI_SHAPE_BCWH, 4, (1), AI_NETWORK_IN_1_CHANNEL, AI_NETWORK_IN_1_HEIGHT, (1)),
//    AI_NETWORK_IN_1_SIZE,                // total elements = 1000
//    NULL,                                // meta_info
//    NULL                                 // data pointer (bind at runtime)
//  )
//};
//static ai_buffer ai_output[AI_NETWORK_OUT_NUM] = {
//  AI_BUFFER_INIT(
//    AI_FLAG_NONE,
//    AI_NETWORK_OUT_1_FORMAT,
//	AI_BUFFER_SHAPE_INIT(AI_SHAPE_BCWH, 4, (1), AI_NETWORK_OUT_1_CHANNEL, AI_NETWORK_OUT_1_SIZE, (1)),
//    AI_NETWORK_OUT_1_SIZE,
//    NULL,
//    NULL
//  )
//};


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
    if (xTaskGetTickCount() - last > pdMS_TO_TICKS(5000)) {
        last = xTaskGetTickCount();
        char buf[64];
        snprintf(buf, sizeof(buf), "Free heap: %u bytes\r\n", xPortGetFreeHeapSize());
        HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
    }
}



/* Initialize AI network */
static void InitAI(void) {
    ai_error err;

    // Assign our activation buffer to the table
    g_network_activations_table[1] = (ai_handle)ai_activations;

    // Create and initialize network in one call
    err = ai_network_create_and_init(
        &ecg_network,
        &g_network_activations_table[1],
        &g_network_weights_table[1]
    );

    if (err.type != AI_ERROR_NONE) {
        char buf[64];
        snprintf(buf, sizeof(buf), "AI Init error: %d\r\n", err.code);
        HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
        ecg_network = AI_HANDLE_NULL;
        return;
    }


    ai_network_report info;
    ai_network_get_info(ecg_network, &info);

    HAL_UART_Transmit(&huart3, (uint8_t*)"Model initialized OK\r\n",
                          strlen("Model initialized OK\r\n"), HAL_MAX_DELAY);

        // 4) Grab the SDK’s own input/output buffer structs:
        in_bufs  = ai_network_inputs_get(ecg_network,  NULL);
        out_bufs = ai_network_outputs_get(ecg_network, NULL);

        // 5) Bind your C arrays into them:
        in_bufs[0].data = AI_HANDLE_PTR(ai_input_buffer);
        in_bufs[0].size = info.inputs[0].size;    // should be 1000

        out_bufs[0].data = AI_HANDLE_PTR(ai_output_buffer);
        out_bufs[0].size = info.outputs[0].size;  // should match your model’s output

        // 6) (Optional) Log the shape back to UART
        char dbg[80];
        snprintf(dbg, sizeof(dbg),
            "I/O bound: in=%lu elems, out=%lu elems\r\n",
            info.inputs[0].size,
            info.outputs[0].size
        );
        HAL_UART_Transmit(&huart3, (uint8_t*)dbg, strlen(dbg), HAL_MAX_DELAY);

//    // point to your C arrays, and tell the SDK exactly how many bytes:
//    ai_input[0].data  = AI_HANDLE_PTR(ai_input_buffer);
//    ai_input[0].size = info.inputs[0].size;
//    //ai_input[0].shape.n_dims = 4;  // <--- ADD THIS
//
//    ai_output[0].data = AI_HANDLE_PTR(ai_output_buffer);
//    ai_output[0].size = info.outputs[0].size;
//    //ai_output[0].shape.n_dims = 4;  // <--- ADD THIS
//
//    char buf[64];
//            snprintf(buf, sizeof(buf),"Model expects %lu elements at input[0] (f32)\n",
//                    info.inputs[0].size);
//            HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
//            snprintf(buf, sizeof(buf), "ai_input.shape: n_batches=%lu, ch=%lu, w=%lu, h=%lu\r\n",
//                   ai_input[0].shape.data[0],
//                   ai_input[0].shape.data[1],
//                   ai_input[0].shape.data[2],
//                   ai_input[0].shape.data[3]);
//            HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);

}

/* Read ADC value */
static inline int16_t ReadADC(void) {
    if (xSemaphoreTake(adcSemaphore, pdMS_TO_TICKS(10)) != pdTRUE)
        return 2048;

    HAL_StatusTypeDef status = HAL_ADC_Start(&hadc1);
    if (status != HAL_OK) {
        HAL_ADC_Stop(&hadc1);
        xSemaphoreGive(adcSemaphore);
        return 2048;
    }

    status = HAL_ADC_PollForConversion(&hadc1, 1);
    if (status != HAL_OK) {
        HAL_ADC_Stop(&hadc1);
        xSemaphoreGive(adcSemaphore);
        return 2048;
    }

    int16_t value = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    xSemaphoreGive(adcSemaphore);
    return value;
}

/* Main ECG sampling & inference task */
void ECGSamplingTask(void *argument) {

    InitAI();
    if(ecg_network == AI_HANDLE_NULL) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"AI Init Failed!\r\n", 16, 100);
        vTaskDelete(NULL);
    }

    TickType_t last_wake = xTaskGetTickCount();
//    char uartBuffer[64];

    while (1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SAMPLE_RATE_MS));

        // Read and preprocess ADC
        int16_t raw = ReadADC();
        float voltage = raw * (3.3f / 4095.0f);
        float centered = (voltage - 1.65f);

        // Store in buffer
        modelInputBuffer[sampleCount++] = centered;

        // Process when buffer full
        if (sampleCount >= MODEL_INPUT_SIZE) {
            float dynamicMean = computeMean(modelInputBuffer, MODEL_INPUT_SIZE);
            float dynamicStd = computeStd(modelInputBuffer, MODEL_INPUT_SIZE, dynamicMean);

            for (volatile int i = 0; i < MODEL_INPUT_SIZE; i++) {
            	ai_input_buffer[i] = (modelInputBuffer[i] - dynamicMean) / (dynamicStd + 1e-8f);
            }

            // Run inference
            __DSB();  // Data synchronization barrier
            __ISB();  // Instruction synchronization barrier
            ai_i32 ret = ai_network_run(ecg_network, in_bufs, out_bufs);
//            ai_i32 ret = ai_network_run(ecg_network, ai_input, ai_output);
            if (ret == AI_NETWORK_OUT_NUM) {
            	ai_float yq = ai_output_buffer[0];

            	int is_anomaly = yq > 0.5f ? 1 : 0;

            	char uartBuffer[64];
            	snprintf(uartBuffer, sizeof(uartBuffer), "Anomaly:%d, Score:%.4f\r\n", is_anomaly, yq);
            	HAL_UART_Transmit(&huart3, (uint8_t*)uartBuffer, strlen(uartBuffer), HAL_MAX_DELAY);
            } else {
                // Detailed error handling
                ai_error err = ai_network_get_error(ecg_network);
                char errBuf[128];

                // Print error code
                snprintf(errBuf, sizeof(errBuf), "Inference failed: code=%d, type=%d\r\n", err.code, err.type);
                HAL_UART_Transmit(&huart3, (uint8_t*)errBuf, strlen(errBuf), HAL_MAX_DELAY);

                // Print input buffer stats
                float min_val = 255, max_val = 0;
                for (volatile int i = 0; i < MODEL_INPUT_SIZE; i++) {
                    if (ai_input_buffer[i] < min_val) min_val = ai_input_buffer[i];
                    if (ai_input_buffer[i] > max_val) max_val = ai_input_buffer[i];
                }
                snprintf(errBuf, sizeof(errBuf), "Input range: min=%.2f, max=%.2f\r\n", min_val, max_val);
                HAL_UART_Transmit(&huart3, (uint8_t*)errBuf, strlen(errBuf), HAL_MAX_DELAY);

                // Print first few samples
                snprintf(errBuf, sizeof(errBuf), "Samples: %.2f, %.2f, %.2f, %.2f, %.2f\r\n",
                         ai_input_buffer[0], ai_input_buffer[1],
                         ai_input_buffer[2], ai_input_buffer[3],
                         ai_input_buffer[4]);
                HAL_UART_Transmit(&huart3, (uint8_t*)errBuf, strlen(errBuf), HAL_MAX_DELAY);
            }

            sampleCount = 0;
            MonitorHeap();
        }
//        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* ECG Task and Semaphore Initialization */
void ECGTask_Init(void) {
    adcSemaphore = xSemaphoreCreateMutex();
    uartSemaphore = xSemaphoreCreateMutex();
}


