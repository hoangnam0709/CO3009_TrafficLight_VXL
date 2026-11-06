/*
 * dht20.c
 *
 *  Created on: Nov 4, 2026
 *      Author: Duy
 */

#include "dht20.h"
#include "i2c_bus.h"

/*
 * Khung truyền (datasheet Aosong DHT20 / AHT20):
 *   Ghi : [0xAC][0x33][0x00]                 -> bắt đầu đo
 *   Chờ : >= 80 ms
 *   Đọc : 7 byte
 *         b0     : status (bit7 = busy, bit3 = đã hiệu chuẩn)
 *         b1..b3 : 20 bit độ ẩm     = b1<<12 | b2<<4 | b3>>4
 *         b3..b5 : 20 bit nhiệt độ  = (b3&0x0F)<<16 | b4<<8 | b5
 *         b6     : CRC8 (poly 0x31, init 0xFF) trên 6 byte đầu
 *   RH (%) = raw_h / 2^20 * 100
 *   T (C)  = raw_t / 2^20 * 200 - 50
 */

/* ---------------------------- Biến nội bộ -------------------------------- */
static uint8_t rx_buf[7];
static uint8_t wait_ticks = 0u;
static uint8_t busy_retry = 0u;
static uint8_t data_ready = 0u;
static uint8_t data_valid = 0u;

volatile DHT20_State_t dht20_state = DHT20_IDLE;
uint8_t  dht20_fail_streak = 0u;
uint32_t dht20_ok_count    = 0u;
uint8_t  dht20_last_status = 0u;

/* ------------------------------- CRC8 ------------------------------------ */
static uint8_t dht20_crc8(const uint8_t *buf, uint8_t len)
{
    uint8_t crc = 0xFFu;
    uint8_t i, b;

    for (i = 0u; i < len; i++) {
        crc ^= buf[i];
        for (b = 0u; b < 8u; b++) {
            if ((crc & 0x80u) != 0u) {
                crc = (uint8_t)((crc << 1) ^ 0x31u);
            } else {
                crc = (uint8_t)(crc << 1);
            }
        }
    }
    return crc;
}

static void dht20_fail(void)
{
    if (dht20_fail_streak < 255u) {
        dht20_fail_streak++;
    }
    if (dht20_fail_streak >= DHT20_MAX_FAIL) {
        data_valid = 0u;                      /* LCD sẽ hiển thị "--" thay vì số cũ */
    }
    dht20_state = DHT20_ERR;
}

static void dht20_send_init(void)
{
    uint8_t cmd[3];

    cmd[0] = DHT20_CMD_INIT;
    cmd[1] = DHT20_ARG_INIT_0;
    cmd[2] = DHT20_ARG_INIT_1;
    (void)i2c_bus_write(DHT20_I2C_ADDRESS, cmd, 3u, DHT20_I2C_TIMEOUT);
}

/* ------------------------------- Khởi tạo -------------------------------- */
/* Gọi 1 lần trước khi chạy scheduler (có dùng HAL_Delay). Trả về 1 nếu thấy sensor. */
uint8_t dht20_init(void)
{
    uint8_t status = 0u;

    HAL_Delay(100);          /* datasheet: chờ >= 100 ms sau khi cấp nguồn */

    wait_ticks  = 0u;
    busy_retry  = 0u;
    data_ready  = 0u;
    data_valid  = 0u;
    dht20_state = DHT20_IDLE;

    if (i2c_bus_probe(DHT20_I2C_ADDRESS) != I2C_BUS_OK) {
        dht20_state = DHT20_ERR;
        return 0u;
    }
    if (i2c_bus_read(DHT20_I2C_ADDRESS, &status, 1u, DHT20_I2C_TIMEOUT) != I2C_BUS_OK) {
        dht20_state = DHT20_ERR;
        return 0u;
    }
    dht20_last_status = status;

    if ((status & DHT20_STATUS_CALIB) == 0u) {   /* chưa hiệu chuẩn -> nạp lệnh BE 08 00 */
        dht20_send_init();
        HAL_Delay(10);
    }
    return 1u;
}

/* ------------------------- Bắt đầu một phép đo --------------------------- */
void dht20_start(void)
{
    uint8_t cmd[3];

    if (dht20_state == DHT20_CONVERTING) {
        return;                               /* đang đợi kết quả, không trigger chồng */
    }

    cmd[0] = DHT20_CMD_TRIGGER;
    cmd[1] = DHT20_ARG_TRIGGER_0;
    cmd[2] = DHT20_ARG_TRIGGER_1;

    if (i2c_bus_write(DHT20_I2C_ADDRESS, cmd, 3u, DHT20_I2C_TIMEOUT) != I2C_BUS_OK) {
        dht20_fail();
        return;
    }

    wait_ticks  = (uint8_t)DHT20_WAIT_TICKS;
    busy_retry  = 0u;
    dht20_state = DHT20_CONVERTING;
}

/* ---------- Gọi mỗi 10 ms: đợi đủ 80 ms rồi đọc và quy đổi ---------------- */
void dht20_process(void)
{
    uint32_t raw_h;
    uint32_t raw_t;

    if (dht20_state != DHT20_CONVERTING) {
        return;
    }
    if (wait_ticks > 0u) {
        wait_ticks--;                         /* chưa đủ 80 ms */
        return;
    }

    if (i2c_bus_read(DHT20_I2C_ADDRESS, rx_buf, 7u, DHT20_I2C_TIMEOUT) != I2C_BUS_OK) {
        dht20_fail();
        return;
    }
    dht20_last_status = rx_buf[0];

    if ((rx_buf[0] & DHT20_STATUS_BUSY) != 0u) {      /* sensor còn bận: đọc lại ở tick sau */
        if (busy_retry < DHT20_BUSY_RETRY) {
            busy_retry++;
            return;
        }
        dht20_fail();
        return;
    }

    if ((rx_buf[0] & DHT20_STATUS_CALIB) == 0u) {     /* mất hiệu chuẩn: nạp lại, bỏ lần đo này */
        dht20_send_init();
        dht20_fail();
        return;
    }

    if (dht20_crc8(rx_buf, 6u) != rx_buf[6]) {        /* sai CRC -> bỏ dữ liệu */
        dht20_fail();
        return;
    }

    raw_h = ((uint32_t)rx_buf[1] << 12)
          | ((uint32_t)rx_buf[2] << 4)
          | ((uint32_t)rx_buf[3] >> 4);

    raw_t = (((uint32_t)rx_buf[3] & 0x0Fu) << 16)
          | ((uint32_t)rx_buf[4] << 8)
          |  (uint32_t)rx_buf[5];

    humidity    = ((float)raw_h * 100.0f) / 1048576.0f;             /* 2^20 */
    temperature = ((float)raw_t * 200.0f) / 1048576.0f - 50.0f;

    dht20_ok_count++;
    dht20_fail_streak = 0u;
    data_valid  = 1u;
    data_ready  = 1u;
    dht20_state = DHT20_OK;
}

/* ------------------------------- Truy vấn -------------------------------- */
uint8_t dht20_data_ready(void)
{
    return data_ready;
}

void dht20_clear_data_ready(void)
{
    data_ready = 0u;
}

uint8_t dht20_is_valid(void)
{
    return data_valid;
}

float dht20_get_temperature(void)
{
    return temperature;
}

float dht20_get_humidity(void)
{
    return humidity;
}
