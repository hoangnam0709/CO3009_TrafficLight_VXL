/*
 * fsm_manual.h
 *
 *  Created on: 6 thg 11, 2026
 *      Author: cuong
 */

#ifndef INC_FSM_MANUAL_H_
#define INC_FSM_MANUAL_H_

#include <stdint.h>

typedef enum {
    MAN_INIT = 0,        /* entry: force a safe state */
    MAN_ALL_RED_A,       /* both red, next -> R1 G2 */
    MAN_RED1_GREEN2,
    MAN_RED1_AMBER2,
    MAN_ALL_RED_B,       /* both red, next -> G1 R2 */
    MAN_GREEN1_RED2,
    MAN_AMBER1_RED2
} manual_state_t;

void    fsm_manual_init(void);          /* call when ENTERING manual mode */
void    fsm_manual_run(void);           /* scheduler task, every 10 ms */
uint8_t fsm_manual_exit_requested(void);/* 1 once if user asked to leave manual mode */
manual_state_t fsm_manual_get_state(void);

#endif /* INC_FSM_MANUAL_H_ */
