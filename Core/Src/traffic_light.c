/*
 * traffic_light.c
 *
 *  Created on: 6 thg 11, 2026
 *      Author: cuong
 */
#include "traffic_light.h"
#include "main.h"

/* Signal (A,B) for each colour, indexed by light_color_t.
 * TODO: fill in from the Step 3.2 observation table. Current assumption:
 *   (0,0)=OFF  (0,1)=RED  (1,0)=GREEN  (1,1)=AMBER                      */
typedef struct { uint8_t a; uint8_t b; } signal_t;

static const signal_t signal_table[4] = {
    /* LIGHT_OFF   */ {0, 0},
    /* LIGHT_RED   */ {0, 1},
    /* LIGHT_GREEN */ {1, 0},
    /* LIGHT_AMBER */ {1, 1}
};

static uint8_t current_color[3] = {LIGHT_OFF, LIGHT_OFF, LIGHT_OFF}; /* [1],[2] used */

void traffic_set(uint8_t road, uint8_t color)
{
    if (color > LIGHT_AMBER) return;

    GPIO_PinState a = signal_table[color].a ? GPIO_PIN_SET : GPIO_PIN_RESET;
    GPIO_PinState b = signal_table[color].b ? GPIO_PIN_SET : GPIO_PIN_RESET;

    switch (road) {
    case ROAD_1:
        HAL_GPIO_WritePin(TL1_A_GPIO_Port, TL1_A_Pin, a);
        HAL_GPIO_WritePin(TL1_B_GPIO_Port, TL1_B_Pin, b);
        break;
    case ROAD_2:
        HAL_GPIO_WritePin(TL2_A_GPIO_Port, TL2_A_Pin, a);
        HAL_GPIO_WritePin(TL2_B_GPIO_Port, TL2_B_Pin, b);
        break;
    default:
        return;
    }
    current_color[road] = color;
}

uint8_t traffic_get(uint8_t road)
{
    if (road != ROAD_1 && road != ROAD_2) return LIGHT_OFF;
    return current_color[road];
}

