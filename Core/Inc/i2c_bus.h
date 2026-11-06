/*
 * i2c_bus.h
 *
 *  Created on: Nov 4, 2026
 *      Author: Duy
 */

#ifndef INC_I2C_BUS_H_
#define INC_I2C_BUS_H_

#include "main.h"
#include <stdint.h>

/*
 * Lớp truy cập I2C1 dùng chung cho DHT20 và LCD (TV3 - feature/i2c-tuning).
 *  - Nhận địa chỉ 7-bit (0x38, 0x27...) và tự dịch trái 1 bit khi gọi HAL,
 *    tránh lỗi "quên shift địa chỉ" (mục B8 của tài liệu đồ án).
 *  - Tự phục hồi bus khi HAL báo BUSY / TIMEOUT (lỗi quen thuộc của I2C trên STM32F1).
 *  - Đây là chỗ DUY NHẤT trong code TV3 gọi HAL_I2C_*.
 */

/* Handle I2C1 được định nghĩa trong main.c (sinh bởi CubeMX) */
extern I2C_HandleTypeDef hi2c1;

#define I2C_BUS_OK      1u      /* thành công */
#define I2C_BUS_ERR     0u      /* lỗi */

uint8_t i2c_bus_write(uint8_t addr7, const uint8_t *data, uint16_t len, uint32_t timeout_ms);
uint8_t i2c_bus_read(uint8_t addr7, uint8_t *data, uint16_t len, uint32_t timeout_ms);
uint8_t i2c_bus_probe(uint8_t addr7);                       /* 1 = thiết bị ACK địa chỉ */
uint8_t i2c_bus_scan(uint8_t *found, uint8_t max_found);    /* quét 0x01..0x7F, trả về số thiết bị */
void    i2c_bus_recover(void);                              /* DeInit + Init lại I2C1 */

#endif /* INC_I2C_BUS_H_ */
