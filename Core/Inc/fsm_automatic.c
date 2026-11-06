#include "fsm_automatic.h"
#include "global.h"
#include "software_timer.h"
#include "button.h"
#include "traffic_light.h"

/* Khai báo weak để build không lỗi khi TV2 chưa code xong button.c & traffic_light.c */
__attribute__((weak)) int isButtonPressed(int index) { return 0; }
__attribute__((weak)) void traffic_set(uint8_t road, uint8_t color) {}

void fsm_automatic_run(void) {
    switch (status) {
        case INIT:
            traffic_set(1, LIGHT_OFF);
            traffic_set(2, LIGHT_OFF);

            status = AUTO_RED_GREEN;
            road1_countdown = time_red;
            road2_countdown = time_green;
            traffic_set(1, LIGHT_RED);
            traffic_set(2, LIGHT_GREEN);

            setTimer(0, time_green * 1000); // Timer 0: thời gian pha hiện tại
            setTimer(1, 1000);              // Timer 1: nhịp đếm ngược 1 giây
            break;

        case AUTO_RED_GREEN:
            traffic_set(1, LIGHT_RED);
            traffic_set(2, LIGHT_GREEN);

            // Đếm ngược mỗi 1 giây cho LCD
            if (timer_flag[1] == 1) {
                setTimer(1, 1000);
                if (road1_countdown > 1) road1_countdown--;
                if (road2_countdown > 1) road2_countdown--;
            }

            // Hết thời gian Xanh đường 2 -> Chuyển sang Đỏ 1 - Vàng 2
            if (timer_flag[0] == 1) {
                status = AUTO_RED_AMBER;
                road1_countdown = time_amber;
                road2_countdown = time_amber;
                setTimer(0, time_amber * 1000);
                setTimer(1, 1000);
            }

            // Nhấn nút 0 (BTN_1 - Mode) -> Chuyển sang chế độ Manual
            if (isButtonPressed(0) == 1) {
                status = MAN_RED_GREEN;
                clearTimer(0);
                clearTimer(1);
            }
            break;

        case AUTO_RED_AMBER:
            traffic_set(1, LIGHT_RED);
            traffic_set(2, LIGHT_AMBER);

            if (timer_flag[1] == 1) {
                setTimer(1, 1000);
                if (road1_countdown > 1) road1_countdown--;
                if (road2_countdown > 1) road2_countdown--;
            }

            // Hết thời gian Vàng đường 2 -> Chuyển sang Xanh 1 - Đỏ 2
            if (timer_flag[0] == 1) {
                status = AUTO_GREEN_RED;
                road1_countdown = time_green;
                road2_countdown = time_red;
                setTimer(0, time_green * 1000);
                setTimer(1, 1000);
            }

            if (isButtonPressed(0) == 1) {
                status = MAN_RED_AMBER;
                clearTimer(0);
                clearTimer(1);
            }
            break;

        case AUTO_GREEN_RED:
            traffic_set(1, LIGHT_GREEN);
            traffic_set(2, LIGHT_RED);

            if (timer_flag[1] == 1) {
                setTimer(1, 1000);
                if (road1_countdown > 1) road1_countdown--;
                if (road2_countdown > 1) road2_countdown--;
            }

            // Hết thời gian Xanh đường 1 -> Chuyển sang Vàng 1 - Đỏ 2
            if (timer_flag[0] == 1) {
                status = AUTO_AMBER_RED;
                road1_countdown = time_amber;
                road2_countdown = time_amber;
                setTimer(0, time_amber * 1000);
                setTimer(1, 1000);
            }

            if (isButtonPressed(0) == 1) {
                status = MAN_GREEN_RED;
                clearTimer(0);
                clearTimer(1);
            }
            break;

        case AUTO_AMBER_RED:
            traffic_set(1, LIGHT_AMBER);
            traffic_set(2, LIGHT_RED);

            if (timer_flag[1] == 1) {
                setTimer(1, 1000);
                if (road1_countdown > 1) road1_countdown--;
                if (road2_countdown > 1) road2_countdown--;
            }

            // Hết thời gian Vàng đường 1 -> Quay lại Đỏ 1 - Xanh 2
            if (timer_flag[0] == 1) {
                status = AUTO_RED_GREEN;
                road1_countdown = time_red;
                road2_countdown = time_green;
                setTimer(0, time_green * 1000);
                setTimer(1, 1000);
            }

            if (isButtonPressed(0) == 1) {
                status = MAN_AMBER_RED;
                clearTimer(0);
                clearTimer(1);
            }
            break;

        default:
            break;
    }
}
