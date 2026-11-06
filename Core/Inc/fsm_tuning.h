/*
 * fsm_tuning.h
 *
 *  Created on: Nov 4, 2026
 *      Author: Duy
 */

#ifndef INC_FSM_TUNING_H_
#define INC_FSM_TUNING_H_

#include "main.h"
#include "global.h"
#include <stdint.h>

/*
 * Máy trạng thái TINH CHỈNH thời gian đèn: chọn màu -> tăng/giảm -> lưu hoặc huỷ.
 *
 * Ràng buộc hợp lệ (2 đường dùng chung một bộ thời gian trong global.c):
 *      RED = GREEN + AMBER
 *      3 <= GREEN <= 30 (s),  1 <= AMBER <= 5 (s)   =>   4 <= RED <= 35 (s)
 * Chuỗi hiển thị trên LCD dùng ASCII (HD44780 không có dấu tiếng Việt).
 *
 * Các module khác chỉ cần gán status = TUNE_RED để vào chế độ này;
 * thoát bằng BTN4 -> status = INIT (2 đường về Đỏ, fsm_automatic khởi tạo lại từ pha đầu).
 */

/* --------------- Chỉ số nút nhấn cho isButtonPressed() của TV2 ------------- */
/* 0 = BTN1 (D7), 1 = BTN2 (D8), 2 = BTN3 (D9), 3 = BTN4 (D10) */
#define TUNE_BTN_INC        0u   /* BTN1: mục kế tiếp / tăng giá trị */
#define TUNE_BTN_DEC        1u   /* BTN2: mục trước đó / giảm giá trị */
#define TUNE_BTN_OK         2u   /* BTN3: vào chế độ sửa / lưu */
#define TUNE_BTN_EXIT       3u   /* BTN4: huỷ khi đang sửa / thoát Tuning */

/* ------------------------------- Giới hạn -------------------------------- */
#define TUNE_GREEN_MIN      3
#define TUNE_GREEN_MAX      30
#define TUNE_AMBER_MIN      1
#define TUNE_AMBER_MAX      5
#define TUNE_RED_MIN        (TUNE_GREEN_MIN + TUNE_AMBER_MIN)   /* 4  */
#define TUNE_RED_MAX        (TUNE_GREEN_MAX + TUNE_AMBER_MAX)   /* 35 */

/* -------------------------------- Trạng thái ----------------------------- */
typedef enum {
    TUNE_STEP_SELECT = 0,   /* đang chọn màu cần chỉnh */
    TUNE_STEP_EDIT   = 1    /* đang tăng/giảm giá trị */
} TuneStep_t;

/* --------------------------------- API ----------------------------------- */
void    fsm_tuning_run(void);        /* task 10 ms của scheduler */
void    fsm_tuning_enter(void);      /* bắt đầu phiên tinh chỉnh (tự gọi từ fsm_tuning_run) */
void    fsm_tuning_exit(void);       /* thoát: 2 đường Đỏ, status = INIT */
uint8_t fsm_tuning_is_active(void);
void    fsm_tuning_display(void);    /* vẽ lại LCD */

/* Giá trị nháp đang chỉnh (mở ra để debug / in UART) */
extern int tune_draft_green;
extern int tune_draft_amber;
extern int tune_draft_red;

#endif /* INC_FSM_TUNING_H_ */
