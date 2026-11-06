/*
 * dht20.h
 *
 *  Created on: Nov 4, 2026
 *      Author: Duy
 */

#ifndef INC_DHT20_H_
#define INC_DHT20_H_

#include "main.h"
#include "global.h"
#include <stdint.h>

/*
 * Cảm biến nhiệt độ / độ ẩm DHT20 (tương thích AHT20) qua I2C1.
 * Đọc KHÔNG blocking: dht20_start() gửi lệnh đo, dht20_process() được scheduler
 * gọi mỗi 10 ms, tự đợi đủ 80 ms rồi mới đọc kết quả.
 * Kết quả ghi vào temperature / humidity trong global.c.
 */

/* ------------------------- Tham số theo datasheet ------------------------- */
#define DHT20_I2C_ADDRESS       0x38u   /* địa chỉ 7-bit (i2c_bus tự dịch trái 1 bit) */

#define DHT20_CMD_TRIGGER       0xACu   /* bắt đầu đo: AC 33 00 */
#define DHT20_ARG_TRIGGER_0     0x33u
#define DHT20_ARG_TRIGGER_1     0x00u
#define DHT20_CMD_INIT          0xBEu   /* nạp hiệu chuẩn: BE 08 00 */
#define DHT20_ARG_INIT_0        0x08u
#define DHT20_ARG_INIT_1        0x00u

#define DHT20_STATUS_BUSY       0x80u   /* bit7 = 1: đang đo */
#define DHT20_STATUS_CALIB      0x08u   /* bit3 = 1: đã hiệu chuẩn */

#define DHT20_CONVERSION_MS     80u     /* thời gian đo tối thiểu */

/* Chu kỳ gọi dht20_process() của scheduler (ms). Đổi nếu tick scheduler khác 10 ms */
#ifndef DHT20_TICK_MS
#define DHT20_TICK_MS           10u
#endif

/* Số lần gọi dht20_process() cần bỏ qua trước khi đọc.
 * Làm tròn lên và bỏ qua lần gọi đầu nên thời gian đo thực tế luôn >= 80 ms
 * dù task chạy lệch pha với lúc dht20_start(). */
#define DHT20_WAIT_TICKS        ((DHT20_CONVERSION_MS + DHT20_TICK_MS - 1u) / DHT20_TICK_MS)

#define DHT20_BUSY_RETRY        3u      /* số lần đọc lại khi sensor còn bận */
#define DHT20_MAX_FAIL          3u      /* quá số lần lỗi liên tiếp này thì coi dữ liệu là không hợp lệ */
#define DHT20_I2C_TIMEOUT       20u     /* ms */

/* ------------------------------- Trạng thái ------------------------------ */
typedef enum {
    DHT20_IDLE = 0,         /* chưa đo */
    DHT20_CONVERTING,       /* đã gửi lệnh đo, đang đợi */
    DHT20_OK,               /* vừa đọc thành công */
    DHT20_ERR               /* lỗi I2C / CRC / sensor chưa sẵn sàng */
} DHT20_State_t;

extern volatile DHT20_State_t dht20_state;
extern uint8_t  dht20_fail_streak;      /* số lần lỗi LIÊN TIẾP (về 0 khi đọc được) */
extern uint32_t dht20_ok_count;         /* tổng số lần đọc thành công */
extern uint8_t  dht20_last_status;      /* byte trạng thái thô lần đọc gần nhất (debug) */

/* --------------------------------- API ----------------------------------- */
uint8_t dht20_init(void);               /* gọi 1 lần trong main(), TRƯỚC khi chạy scheduler */
void    dht20_start(void);              /* task 2000 ms: gửi lệnh đo mới */
void    dht20_process(void);            /* task 10 ms: đợi 80 ms, đọc 7 byte, kiểm CRC */
uint8_t dht20_data_ready(void);         /* 1 = có dữ liệu mới chưa được hiển thị */
void    dht20_clear_data_ready(void);
uint8_t dht20_is_valid(void);           /* 1 = đã có số liệu tốt và cảm biến không lỗi liên tục */
float   dht20_get_temperature(void);    /* °C */
float   dht20_get_humidity(void);       /* %RH */

#endif /* INC_DHT20_H_ */
