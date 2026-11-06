/*
 * button.c
 *
 *  Created on: 6 thg 11, 2026
 *      Author: cuong
 */


#include "button.h"
#include "main.h"

/* ---- Configuration ---- */
#define BUTTON_PRESSED_LEVEL   GPIO_PIN_RESET   /* TODO: verify in Step 3.3 */
#define BUTTON_RELEASED_LEVEL  GPIO_PIN_SET
#define LONG_PRESS_TICKS       100              /* 100 x 10 ms = 1 s */

/* ---- Hardware table (only place that touches HAL/GPIO) ---- */
static GPIO_TypeDef *const btn_port[NO_OF_BUTTONS] = {
    BTN_1_GPIO_Port, BTN_2_GPIO_Port, BTN_3_GPIO_Port, BTN_4_GPIO_Port
};
static const uint16_t btn_pin[NO_OF_BUTTONS] = {
    BTN_1_Pin, BTN_2_Pin, BTN_3_Pin, BTN_4_Pin
};

/* ---- Debounce state ---- */
static GPIO_PinState reg0[NO_OF_BUTTONS];   /* newest sample */
static GPIO_PinState reg1[NO_OF_BUTTONS];
static GPIO_PinState reg2[NO_OF_BUTTONS];   /* oldest sample */
static GPIO_PinState stable[NO_OF_BUTTONS]; /* last debounced level */

static uint16_t hold_counter[NO_OF_BUTTONS];
static uint8_t  pressed_flag[NO_OF_BUTTONS];
static uint8_t  long_flag[NO_OF_BUTTONS];

void button_init(void)
{
    for (uint8_t i = 0; i < NO_OF_BUTTONS; i++) {
        reg0[i] = reg1[i] = reg2[i] = stable[i] = BUTTON_RELEASED_LEVEL;
        hold_counter[i] = 0;
        pressed_flag[i] = 0;
        long_flag[i] = 0;
    }
}

/* Call every 10 ms */
void button_read(void)
{
    for (uint8_t i = 0; i < NO_OF_BUTTONS; i++) {
        reg2[i] = reg1[i];
        reg1[i] = reg0[i];
        reg0[i] = HAL_GPIO_ReadPin(btn_port[i], btn_pin[i]);

        /* Accept a level only after 3 identical consecutive samples (30 ms) */
        if (reg0[i] == reg1[i] && reg1[i] == reg2[i]) {
            if (stable[i] != reg0[i]) {              /* edge detected */
                stable[i] = reg0[i];
                if (stable[i] == BUTTON_PRESSED_LEVEL) {
                    pressed_flag[i] = 1;
                    hold_counter[i] = LONG_PRESS_TICKS;
                } else {
                    hold_counter[i] = 0;
                }
            } else if (stable[i] == BUTTON_PRESSED_LEVEL) {  /* still held */
                if (hold_counter[i] > 0) {
                    hold_counter[i]--;
                    if (hold_counter[i] == 0) {
                        long_flag[i] = 1;            /* fires once per hold */
                    }
                }
            }
        }
    }
}

uint8_t isButtonPressed(uint8_t index)
{
    if (index >= NO_OF_BUTTONS) return 0;
    if (pressed_flag[index]) {
        pressed_flag[index] = 0;
        return 1;
    }
    return 0;
}

uint8_t isButtonLongPressed(uint8_t index)
{
    if (index >= NO_OF_BUTTONS) return 0;
    if (long_flag[index]) {
        long_flag[index] = 0;
        return 1;
    }
    return 0;
}

uint8_t isButtonHeld(uint8_t index)
{
    if (index >= NO_OF_BUTTONS) return 0;
    return (stable[index] == BUTTON_PRESSED_LEVEL);
}
