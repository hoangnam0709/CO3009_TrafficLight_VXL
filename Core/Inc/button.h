/*
 * button.h
 *
 *  Created on: 6 thg 11, 2026
 *      Author: cuong
 */

#ifndef INC_BUTTON_H_
#define INC_BUTTON_H_

#include <stdint.h>

#define NO_OF_BUTTONS      4

#define BUTTON_1           0   /* BTN_1 - D7  - PA8 */
#define BUTTON_2           1   /* BTN_2 - D8  - PA9 */
#define BUTTON_3           2   /* BTN_3 - D9  - PC7 */
#define BUTTON_4           3   /* BTN_4 - D10 - PB6 */

void button_init(void);
void button_read(void);                   /* run as a scheduler task every 10 ms */

uint8_t isButtonPressed(uint8_t index);       /* 1 once per short press (flag auto-cleared) */
uint8_t isButtonLongPressed(uint8_t index);   /* 1 once per long press (flag auto-cleared) */
uint8_t isButtonHeld(uint8_t index);          /* 1 while the button is stably held down */

#endif /* INC_BUTTON_H_ */
