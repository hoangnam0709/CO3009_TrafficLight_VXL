#ifndef INC_SOFTWARE_TIMER_H_
#define INC_SOFTWARE_TIMER_H_

#define MAX_TIMERS  10
#define TIMER_TICK  10 // Chu kỳ ngắt TIM2 là 10ms

/*
 * Quy ước các kênh Timer dùng chung:
 * Timer 0: Đếm thời gian chuyển pha đèn (Red / Green / Amber)
 * Timer 1: Đếm nhịp 1 giây (1000ms) để giảm biến road1_countdown, road2_countdown
 * Timer 2: Dự phòng cho nhấp nháy LED hoặc timeout
 */
extern int timer_flag[MAX_TIMERS];
extern int timer_counter[MAX_TIMERS];

void setTimer(int index, int duration_ms);
void clearTimer(int index);
void timerRun(void);

#endif /* INC_SOFTWARE_TIMER_H_ */
