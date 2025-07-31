
/*
 * ecg_driver_final.c
 *
 *  Created on: Jul 30, 2025
 *      Author: Yathi
 */

#include "main.h"
#include "FreeRTOS.h"
//#include "ai_datatypes_defines.h"
//#include "ecg_network.h"
//#include "ecg_network_data.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "task.h"
#include "queue.h"
#include "semphr.h"
//#include "ai_platform.h"
//#include "stai.h"

#define MODEL_INPUT_SIZE     1000
#define AI_INPUT_SCALE       0.030224763f
#define AI_INPUT_ZERO_POINT  116
#define AI_OUTPUT_SCALE      0.00390625f
#define AI_OUTPUT_ZERO_POINT 0
#define SAMPLE_RATE_MS       1  // 1ms = 1000Hz sampling

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

AI_ALIGNED(4) static ai_u8 ai_activations[AI_ECG_NETWORK_DATA_ACTIVATIONS_SIZE];

/* AI network and buffers */
//static ai_buffer* ai_input = NULL;
//static ai_buffer* ai_output = NULL;
static ai_handle ecg_network = AI_HANDLE_NULL;
//static ai_buffer ai_input[AI_ECGAI_NETWORK_IN_NUM] = {0};
//static ai_buffer ai_output[AI_ECGAI_NETWORK_OUT_NUM] = {0};

/* Input and output buffers */
AI_ALIGNED(4) static ai_u8 ai_input_buffer[AI_ECG_NETWORK_IN_1_SIZE_BYTES];
AI_ALIGNED(4) static ai_u8 ai_output_buffer[AI_ECG_NETWORK_OUT_1_SIZE_BYTES];


// Use generated weights and activations tables
    extern ai_handle g_ecg_network_weights_table[];
    extern ai_handle g_ecg_network_activations_table[];



/* AI buffer definitions */
static ai_buffer ai_input[AI_ECG_NETWORK_IN_NUM] = {
    AI_BUFFER_OBJ_INIT(
    	STAI_FORMAT_U8,  // format_
        1,
		MODEL_INPUT_SIZE,
        1,
        1,
		ai_input_buffer      // data_
    )
};

static ai_buffer ai_output[AI_ECG_NETWORK_OUT_NUM] = {
    AI_BUFFER_OBJ_INIT(
    	STAI_FORMAT_U8,  // format_
        1,                    // h_ (height: 1)
        1,                    // w_ (width: 1)
        1,                    // ch_ (channels: 1)
        1,                    // n_batches_ (batch size: 1)
		ai_output_buffer      // data_
    )
};

//    /* Input buffer */
//    static ai_buffer ai_input[AI_ECG_NETWORK_IN_NUM] = AI_BUFFER_INIT(
//    	    0,                        // flags
//    	    AI_BUFFER_FORMAT_FLOAT,   // format
//    	    AI_PACK(3, 1, 1000, 1), // shape: [1, 1000, 1]
//    	    1000 * sizeof(ai_float),  // size: 4000 bytes
//    	    NULL,                     // meta_info
//			ai_input_buffer                // data pointer
//    	);
//
//    /* Output buffer */
//    static ai_buffer ai_output[AI_ECG_NETWORK_OUT_NUM] = AI_BUFFER_INIT(
//    	    0,                        // flags
//    	    AI_BUFFER_FORMAT_FLOAT,   // format
//    	    AI_PACK(3, 1, 1, 1), // shape: [1, 1, 1]
//    	    1 * sizeof(ai_float),  // size: 4 bytes
//    	    NULL,                     // meta_info
//			ai_output_buffer                // data pointer
//    	);

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
//static void InitAI(void) {
//    ai_error err;
//
//    // Initialize network structure
//    const ai_network_params params = {
//        .activations = ai_activations,
//        .activations_size = sizeof(ai_activations)
//    };
//
//    // Create and initialize network
//    err = ai_ecgai_network_create(&ecg_network, AI_BUFFER_OBJ_INIT(&params));
//    if (err.type != AI_ERROR_NONE) goto init_failed;
//
//    err = ai_ecgai_network_init(ecg_network);
//    if (err.type != AI_ERROR_NONE) goto init_failed;
//
//    // Get input/output buffers
//    if (ai_ecgai_network_get_inputs(ecg_network, ai_input) != AI_ECGAI_NETWORK_IN_NUM)
//        goto init_failed;
//
//    if (ai_ecgai_network_get_outputs(ecg_network, ai_output) != AI_ECGAI_NETWORK_OUT_NUM)
//        goto init_failed;
//
//    return;
//
//init_failed:
//    char buf[64];
//    snprintf(buf, sizeof(buf), "AI Init error: %d\r\n", err.code);
//    HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
//    ecg_network = AI_HANDLE_NULL;
//}

/* Initialize AI network */
/* Initialize AI network */
//static void InitAI(void) {
//    ai_error err;
//
//    // Create network instance
//    err = ai_ecgai_network_create(&ecg_network, NULL, NULL);
//    if (err.type != AI_ERROR_NONE) {
//        char buf[64];
//        snprintf(buf, sizeof(buf), "Create error: %d\r\n", err.code);
//        HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
//        goto init_failed;
//    }
//
//    // Initialize network with activation buffer
//    if (ai_ecgai_network_init(ecg_network, ai_activations) != true) {
//        char buf[] = "Init failed\r\n";
//        HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
//        ai_ecgai_network_destroy(ecg_network);
//        goto init_failed;
//    }
//
//    // Get input/output buffers using generated functions
//    ai_input = ai_ecgai_network_inputs_get(ecg_network);
//    ai_output = ai_ecgai_network_outputs_get(ecg_network);
//
//    if (!ai_input || !ai_output) {
//        char buf[] = "IO buffers failed\r\n";
//        HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
//        ai_ecgai_network_destroy(ecg_network);
//        goto init_failed;
//    }
//
//    // Assign data buffers
//    ai_input[0].data = AI_HANDLE_PTR(ai_input_buffer);
//    ai_output[0].data = AI_HANDLE_PTR(ai_output_buffer);
//
//    return;
//
//init_failed:
//    ecg_network = AI_HANDLE_NULL;
//}

///* Initialize AI network */
//static void InitAI(void) {
//    ai_error err;
//
//    // Create network instance
//    err = ai_ecgai_network_create(&ecg_network, NULL);
//    if (err.type != AI_ERROR_NONE) {
//        char buf[64];
//        snprintf(buf, sizeof(buf), "Create error: %d\r\n", err.code);
//        HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
//        goto init_failed;
//    }
//
//    // Prepare network parameters
//    const ai_network_params params = {
//        .activations = {
//            .format = AI_BUFFER_FORMAT_U8,
//            .data = AI_HANDLE_PTR(ai_activations),
//            .size = sizeof(ai_activations)
//        }
//    };
//    // After params initialization
//    char dbg_buf[128];
//    snprintf(dbg_buf, sizeof(dbg_buf),
//             "Params: data=%p, size=%lu, align=%d\r\n",
//             ai_activations,
//             sizeof(ai_activations),
//             (int)((uintptr_t)ai_activations % 4));
//    HAL_UART_Transmit(&huart3, (uint8_t*)dbg_buf, strlen(dbg_buf), HAL_MAX_DELAY);
//
//    // Initialize network
//    if (ai_ecgai_network_init(ecg_network, &params) != true) {
//        char buf[] = "Init failed\r\n";
//        HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
//        ai_ecgai_network_destroy(ecg_network);
//        goto init_failed;
//    }
//
//    // After network create
//    snprintf(dbg_buf, sizeof(dbg_buf), "Network handle: %p\r\n", ecg_network);
//    HAL_UART_Transmit(&huart3, (uint8_t*)dbg_buf, strlen(dbg_buf), HAL_MAX_DELAY);
//
//    // Get input buffers
//    ai_u16 n_inputs = 0;
//    ai_input = ai_ecgai_network_inputs_get(ecg_network, &n_inputs);
//    if (!ai_input || n_inputs != AI_ECGAI_NETWORK_IN_NUM) {
//        char buf[64];
//        snprintf(buf, sizeof(buf), "Input count error: %d\r\n", n_inputs);
//        HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
//        ai_ecgai_network_destroy(ecg_network);
//        goto init_failed;
//    }
//
//    // Get output buffers
//    ai_u16 n_outputs = 0;
//    ai_output = ai_ecgai_network_outputs_get(ecg_network, &n_outputs);
//    if (!ai_output || n_outputs != AI_ECGAI_NETWORK_OUT_NUM) {
//        char buf[64];
//        snprintf(buf, sizeof(buf), "Output count error: %d\r\n", n_outputs);
//        HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
//        ai_ecgai_network_destroy(ecg_network);
//        goto init_failed;
//    }
//
//    // Assign data buffers
//    ai_input[0].data = AI_HANDLE_PTR(ai_input_buffer);
//    ai_output[0].data = AI_HANDLE_PTR(ai_output_buffer);
//
//    return;
//
//init_failed:
//    ecg_network = AI_HANDLE_NULL;
//}

/* Initialize AI network */
static void InitAI(void) {
    ai_error err;

    // Assign our activation buffer to the table
    g_ecg_network_activations_table[0] = (ai_handle)ai_activations;

    // Create and initialize network in one call
    err = ai_ecg_network_create_and_init(
        &ecg_network,
        g_ecg_network_activations_table,
        g_ecg_network_weights_table
    );

    if (err.type != AI_ERROR_NONE) {
        char buf[64];
        snprintf(buf, sizeof(buf), "AI Init error: %d\r\n", err.code);
        HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
        ecg_network = AI_HANDLE_NULL;
        return;
    }

    // Get IO buffers using generated functions
//    ai_input = ai_ecgai_network_inputs_get(ecg_network, NULL);
//    ai_output = ai_ecgai_network_outputs_get(ecg_network, NULL);

    // Assign data buffers
//    if(ai_input && ai_output) {
        ai_input[0].data = AI_HANDLE_PTR(ai_input_buffer);
        ai_output[0].data = AI_HANDLE_PTR(ai_output_buffer);
//    }
        // After initialization
        /* After buffer initialization in InitAI */
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "Input buffer: format=%ld, size=%lu, shape_size=%u\r\n",
                 ai_input[0].format,
                 ai_input[0].size,
                 ai_input[0].shape.size);
        HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);

        snprintf(buf, sizeof(buf),
                 "Output buffer: format=%ld, size=%lu, shape_size=%u\r\n",
                 ai_output[0].format,
                 ai_output[0].size,
                 ai_output[0].shape.size);
        HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
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

	/* Before InitAI */
	#if defined(DEBUG)
	    // Test the buffer initialization macro
	    ai_buffer test_buffer = AI_BUFFER_OBJ_INIT(
	        AI_BUFFER_FORMAT_U8,
	        1,
	        1000,
	        1,
	        1,
	        NULL
	    );

	    char test_buf[128];
	    snprintf(test_buf, sizeof(test_buf),
	             "Test buffer: size=%lu, shape_size=%u\r\n",
	             test_buffer.size,
	             test_buffer.shape.size);
	    HAL_UART_Transmit(&huart3, (uint8_t*)test_buf, strlen(test_buf), HAL_MAX_DELAY);
	#endif


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
        float centered = __CLAMP((voltage - 1.65f), -1.65f, 1.65f);

        // Store in buffer
        modelInputBuffer[sampleCount++] = centered;

        // Process when buffer full
        if (sampleCount >= MODEL_INPUT_SIZE) {
            float dynamicMean = computeMean(modelInputBuffer, MODEL_INPUT_SIZE);
            float dynamicStd = computeStd(modelInputBuffer, MODEL_INPUT_SIZE, dynamicMean);
//            uint8_t min_val = 255, max_val = 0;

            // Preprocess and quantize input
//            for (int i = 0; i < MODEL_INPUT_SIZE; i++) {
//                float normalized = (modelInputBuffer[i] - dynamicMean) / (dynamicStd + 1e-8f);
//                int8_t q = (int8_t)roundf((normalized / AI_INPUT_SCALE) + AI_INPUT_ZERO_POINT);
//
//
//                ai_input_buffer[i] = (ai_u8)__CLAMP(q, 0, 255);
//
////                /* Track min/max for debugging */
////                if (ai_input_buffer[i] < min_val) min_val = ai_input_buffer[i];
////                if (ai_input_buffer[i] > max_val) max_val = ai_input_buffer[i];
//
//            }

            for (int i = 0; i < MODEL_INPUT_SIZE; i++) {
                float normalized = (modelInputBuffer[i] - dynamicMean) / (dynamicStd + 1e-8f);
                float quantized = (normalized / AI_INPUT_SCALE) + AI_INPUT_ZERO_POINT;
                ai_input_buffer[i] = (ai_u8)__CLAMP(roundf(quantized), 0, 255);

                // Debug first few values
                if (i < 4) {
                    char debugBuf[128];
                    snprintf(debugBuf, sizeof(debugBuf),
                             "Sample[%d]: raw=%.4f, norm=%.4f, quant=%.4f, final=%u\r\n",
                             i, modelInputBuffer[i], normalized, quantized, ai_input_buffer[i]);
                    HAL_UART_Transmit(&huart3, (uint8_t*)debugBuf, strlen(debugBuf), HAL_MAX_DELAY);
                }
            }

            for (int i = 0; i < MODEL_INPUT_SIZE; i++) {
                ai_input_buffer[i] = (ai_u8)116; // Zero-point value
            }

            // Run inference
            __DSB();  // Data synchronization barrier
            __ISB();  // Instruction synchronization barrier
            ai_i32 ret = ai_ecg_network_run(ecg_network, ai_input, ai_output);
            if (ret == AI_ECG_NETWORK_OUT_NUM) {
            	ai_u8 yq = ai_output_buffer[0];
            	float score = ((float)yq - AI_OUTPUT_ZERO_POINT) * AI_OUTPUT_SCALE;
            	int is_anomaly = score > 0.5f ? 1 : 0;

            	char uartBuffer[64];
            	snprintf(uartBuffer, sizeof(uartBuffer),
            	              "Anomaly:%d, Score:%.4f\r\n", is_anomaly, score);
            	HAL_UART_Transmit(&huart3, (uint8_t*)uartBuffer, strlen(uartBuffer), HAL_MAX_DELAY);
            } else {
                // Detailed error handling
                ai_error err = ai_ecg_network_get_error(ecg_network);
                char errBuf[128];

                // Print error code
                snprintf(errBuf, sizeof(errBuf),
                         "Inference failed: code=%d, type=%d\r\n", err.code, err.type);
                HAL_UART_Transmit(&huart3, (uint8_t*)errBuf, strlen(errBuf), HAL_MAX_DELAY);

                // Print input buffer stats
                uint8_t min_val = 255, max_val = 0;
                for (int i = 0; i < MODEL_INPUT_SIZE; i++) {
                    if (ai_input_buffer[i] < min_val) min_val = ai_input_buffer[i];
                    if (ai_input_buffer[i] > max_val) max_val = ai_input_buffer[i];
                }
                snprintf(errBuf, sizeof(errBuf),
                         "Input range: min=%u, max=%u\r\n", min_val, max_val);
                HAL_UART_Transmit(&huart3, (uint8_t*)errBuf, strlen(errBuf), HAL_MAX_DELAY);

                // Print first few samples
                snprintf(errBuf, sizeof(errBuf),
                         "Samples: %u, %u, %u, %u, %u\r\n",
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

/* Stack Overflow Hook */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    char buf[64];
    snprintf(buf, sizeof(buf), "Stack overflow in %s\r\n", pcTaskName);
    HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
    taskDISABLE_INTERRUPTS();
    while (1);
}
