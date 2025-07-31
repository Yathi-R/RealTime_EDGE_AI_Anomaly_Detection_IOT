/*
 * Max30102_driver.c
 *
 *  Created on: Jul 27, 2025
 *      Author: Yathi
 */

#include "Max30102.h"
#include <string.h>

/* Low‑level I2C wrappers */
static HAL_StatusTypeDef writeReg(uint8_t reg, uint8_t val) {
    return HAL_I2C_Mem_Write(MAX30102_I2C, MAX30102_ADDR, reg,
                             I2C_MEMADD_SIZE_8BIT, &val, 1, 100);
}
static HAL_StatusTypeDef readBytes(uint8_t reg, uint8_t *p, uint8_t len) {
    return HAL_I2C_Mem_Read(MAX30102_I2C, MAX30102_ADDR, reg,
                            I2C_MEMADD_SIZE_8BIT, p, len, 100);
}

bool MAX30102_Init(I2C_HandleTypeDef *hi2c) {
    MAX30102_I2C = hi2c;
    HAL_Delay(100);
    if (writeReg(REG_MODE_CONFIG, 0x40) != HAL_OK) return false;  // reset
    HAL_Delay(100);
    if (writeReg(REG_FIFO_CONFIG, (2<<5)) != HAL_OK) return false; // avg=4
    if (writeReg(REG_SPO2_CONFIG, (0x03<<2)|0x03) != HAL_OK) return false; //100Hz,18‑bit
    if (writeReg(REG_LED1_PA, 0x24) != HAL_OK) return false;       // ~12.6mA
    if (writeReg(REG_LED2_PA, 0x24) != HAL_OK) return false;
    MAX30102_ResetFilter();
    return true;
}

void MAX30102_ResetFilter(void) {
    buf_idx = 0;
    memset(red_buf, 0, sizeof(red_buf));
    memset(ir_buf,  0, sizeof(ir_buf));
}

bool MAX30102_ReadSample(uint32_t *red, uint32_t *ir) {
    uint8_t raw[6];
    uint32_t r, i;
    if (readBytes(REG_FIFO_DATA, raw, 6) != HAL_OK) return false;
    r = (((uint32_t)raw[0]<<16)|((uint32_t)raw[1]<<8)|raw[2]) & 0x3FFFF;
    i = (((uint32_t)raw[3]<<16)|((uint32_t)raw[4]<<8)|raw[5]) & 0x3FFFF;

    /* Only skip if truly no finger: */
    if (r < MAX30102_FINGER_THRESH || i < MAX30102_FINGER_THRESH) {
        return false;
    }

    /* Insert into MA buffer */
    red_buf[buf_idx] = r;
    ir_buf [buf_idx] = i;
    buf_idx = (buf_idx + 1) % MAX30102_MA_SIZE;

    /* Compute moving average */
    uint32_t sum_r = 0, sum_i = 0;
    for (uint8_t j = 0; j < MAX30102_MA_SIZE; j++) {
        sum_r += red_buf[j];
        sum_i += ir_buf[j];
    }
    *red = sum_r / MAX30102_MA_SIZE;
    *ir  = sum_i / MAX30102_MA_SIZE;
    return true;
}

