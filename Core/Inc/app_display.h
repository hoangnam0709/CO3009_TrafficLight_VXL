/*
 * app_display.h
 *
 *  Created on: Nov 4, 2026
 *      Author: Duy
 */

#ifndef INC_APP_DISPLAY_H_
#define INC_APP_DISPLAY_H_

#include "main.h"
#include "global.h"
#include <stdint.h>

/*
 * Task hiển thị LCD chu kỳ 200 ms (TV3 giữ vì sở hữu module lcd).
 *  Dòng 1: nhiệt độ / độ ẩm từ DHT20 ("--" khi cảm biến lỗi hoặc chưa có số liệu).
 *  Dòng 2: chế độ hiện tại (AUTO: đếm ngược 2 đường, MAN: màu đèn hiện tại).
 *  Ở chế độ Tuning task này nhường màn hình cho fsm_tuning.
 */

#define APP_DISPLAY_PERIOD_MS   200u    /* chu kỳ đăng ký với scheduler */

void app_display_init(void);   /* gọi 1 lần trong main() trước khi chạy scheduler */
void app_display_run(void);    /* task 200 ms */

#endif /* INC_APP_DISPLAY_H_ */
