#ifndef INC_GLOBAL_H_
#define INC_GLOBAL_H_

#include "main.h"
#include <stdint.h>

/* 1. CÁC TRẠNG THÁI HỆ THỐNG (Biến status dùng chung cho 3 FSM) */
#define INIT                0
// Chế độ Automatic (TV1)
#define AUTO_RED_GREEN      1
#define AUTO_RED_AMBER      2
#define AUTO_GREEN_RED      3
#define AUTO_AMBER_RED      4
// Chế độ Manual (TV2)
#define MAN_RED_GREEN       11
#define MAN_RED_AMBER       12
#define MAN_GREEN_RED       13
#define MAN_AMBER_RED       14
// Chế độ Tuning (TV3)
#define TUNE_RED            21
#define TUNE_AMBER          22
#define TUNE_GREEN          23

/* 2. MÃ MÀU ĐÈN GIAO THÔNG (Dùng cho hàm traffic_set của TV2) */
#define LIGHT_OFF           0
#define LIGHT_RED           1
#define LIGHT_AMBER         2
#define LIGHT_GREEN         3

/* 3. BIẾN TOÀN CỤC DÙNG CHUNG (Khai báo extern) */
extern int status;
extern int time_red;        // Thời gian đèn Đỏ (mặc định 5s)
extern int time_amber;      // Thời gian đèn Vàng (mặc định 2s)
extern int time_green;      // Thời gian đèn Xanh (mặc định 3s)

extern int road1_countdown; // Đếm ngược hiển thị LCD cho đường 1
extern int road2_countdown; // Đếm ngược hiển thị LCD cho đường 2

extern float temperature;   // Nhiệt độ từ DHT20 (°C)
extern float humidity;      // Độ ẩm từ DHT20 (%)

#endif /* INC_GLOBAL_H_ */
