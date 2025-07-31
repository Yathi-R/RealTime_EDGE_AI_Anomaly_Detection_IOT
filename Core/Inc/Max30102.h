/*
 * Max30102.h
 *
 *  Created on: Jul 27, 2025
 *      Author: Yathi
 */
#ifndef __MAX30102_H
#define __MAX30102_H

#include "stm32h5xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

/* I2C handle and address */
extern I2C_HandleTypeDef *MAX30102_I2C;
#define MAX30102_ADDR           (0x57 << 1)

/* Filter settings */
#define MAX30102_MA_SIZE        8       // moving‑average window
#define MAX30102_FINGER_THRESH  50000U  // below = no finger

/* Registers */
#define REG_FIFO_DATA        0x07
#define REG_MODE_CONFIG      0x09
#define REG_SPO2_CONFIG      0x0A
#define REG_LED1_PA          0x0C
#define REG_LED2_PA          0x0D
#define REG_FIFO_CONFIG      0x08

/* Public API */
bool    MAX30102_Init(I2C_HandleTypeDef *hi2c);
bool    MAX30102_ReadSample(uint32_t *red, uint32_t *ir);
void    MAX30102_ResetFilter(void);

/* I2C handle pointer */
I2C_HandleTypeDef *MAX30102_I2C;

/* Internal buffer */
static uint32_t red_buf[MAX30102_MA_SIZE];
static uint32_t ir_buf [MAX30102_MA_SIZE];
static uint8_t  buf_idx;

#endif /* __MAX30102_H */
