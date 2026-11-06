/*
 * i2c_bus.c
 *
 *  Created on: Nov 4, 2026
 *      Author: Duy
 */

#include "i2c_bus.h"

/* Khởi tạo lại ngoại vi I2C1 với đúng cấu hình CubeMX đã nạp trong hi2c1.Init.
 * HAL_I2C_DeInit() đưa state về RESET nên HAL_I2C_Init() sẽ gọi lại MspInit
 * (bật clock, cấu hình chân SCL/SDA, remap nếu có). */
void i2c_bus_recover(void)
{
    (void)HAL_I2C_DeInit(&hi2c1);
    (void)HAL_I2C_Init(&hi2c1);
}

uint8_t i2c_bus_write(uint8_t addr7, const uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    HAL_StatusTypeDef st;
    uint16_t addr8 = (uint16_t)((uint16_t)addr7 << 1);

    st = HAL_I2C_Master_Transmit(&hi2c1, addr8, (uint8_t *)data, len, timeout_ms);
    if (st == HAL_BUSY) {                 /* bus còn kẹt từ lần lỗi trước */
        i2c_bus_recover();
        st = HAL_I2C_Master_Transmit(&hi2c1, addr8, (uint8_t *)data, len, timeout_ms);
    }
    if (st == HAL_TIMEOUT) {
        i2c_bus_recover();
    }
    return (st == HAL_OK) ? I2C_BUS_OK : I2C_BUS_ERR;
}

uint8_t i2c_bus_read(uint8_t addr7, uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    HAL_StatusTypeDef st;
    uint16_t addr8 = (uint16_t)((uint16_t)addr7 << 1);

    st = HAL_I2C_Master_Receive(&hi2c1, addr8, data, len, timeout_ms);
    if (st == HAL_BUSY) {
        i2c_bus_recover();
        st = HAL_I2C_Master_Receive(&hi2c1, addr8, data, len, timeout_ms);
    }
    if (st == HAL_TIMEOUT) {
        i2c_bus_recover();
    }
    return (st == HAL_OK) ? I2C_BUS_OK : I2C_BUS_ERR;
}

uint8_t i2c_bus_probe(uint8_t addr7)
{
    HAL_StatusTypeDef st;
    uint16_t addr8 = (uint16_t)((uint16_t)addr7 << 1);

    st = HAL_I2C_IsDeviceReady(&hi2c1, addr8, 2u, 10u);
    if (st == HAL_BUSY) {
        i2c_bus_recover();
        st = HAL_I2C_IsDeviceReady(&hi2c1, addr8, 2u, 10u);
    }
    return (st == HAL_OK) ? I2C_BUS_OK : I2C_BUS_ERR;
}

/* Dùng cho bước 3.4 của đồ án: quét bus, ghi các địa chỉ 7-bit tìm được vào found[] */
uint8_t i2c_bus_scan(uint8_t *found, uint8_t max_found)
{
    uint8_t addr;
    uint8_t n = 0u;

    for (addr = 1u; addr < 128u; addr++) {
        if (i2c_bus_probe(addr) == I2C_BUS_OK) {
            if ((found != 0) && (n < max_found)) {
                found[n] = addr;
            }
            n++;
        }
    }
    return n;
}
