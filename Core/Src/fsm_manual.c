/*
 * fsm_manual.c
 *
 *  Created on: 6 thg 11, 2026
 *      Author: cuong
 */
#include "fsm_manual.h"
#include "button.h"
#include "traffic_light.h"

/* Button roles in manual mode */
#define MAN_BTN_EXIT      BUTTON_1   /* leave manual mode (handled by the mode FSM) */
#define MAN_BTN_NEXT      BUTTON_2   /* advance to next safe step */
#define MAN_BTN_ALLRED    BUTTON_3   /* emergency: force both roads red */

static manual_state_t state = MAN_INIT;
static uint8_t exit_flag = 0;

void fsm_manual_init(void)
{
    state = MAN_INIT;
    exit_flag = 0;
}

uint8_t fsm_manual_exit_requested(void)
{
    if (exit_flag) { exit_flag = 0; return 1; }
    return 0;
}

manual_state_t fsm_manual_get_state(void) { return state; }

/* Apply lights for the current state. Never sets both roads GREEN. */
static void apply_state(void)
{
    switch (state) {
    case MAN_ALL_RED_A:
    case MAN_ALL_RED_B:
        traffic_set(ROAD_1, RED);   traffic_set(ROAD_2, RED);   break;
    case MAN_RED1_GREEN2:
        traffic_set(ROAD_1, RED);   traffic_set(ROAD_2, GREEN); break;
    case MAN_RED1_AMBER2:
        traffic_set(ROAD_1, RED);   traffic_set(ROAD_2, AMBER); break;
    case MAN_GREEN1_RED2:
        traffic_set(ROAD_1, GREEN); traffic_set(ROAD_2, RED);   break;
    case MAN_AMBER1_RED2:
        traffic_set(ROAD_1, AMBER); traffic_set(ROAD_2, RED);   break;
    default: break;
    }
}

static void go_to(manual_state_t s)
{
    state = s;
    apply_state();
}

void fsm_manual_run(void)
{
    /* Mode exit has top priority */
    if (isButtonPressed(MAN_BTN_EXIT)) {
        exit_flag = 1;
    }

    /* Emergency all-red: always allowed, always safe */
    if (isButtonPressed(MAN_BTN_ALLRED)) {
        go_to(MAN_ALL_RED_A);
        return;
    }

    switch (state) {
    case MAN_INIT:
        go_to(MAN_ALL_RED_A);      /* entering manual: both red */
        break;

    case MAN_ALL_RED_A:
        if (isButtonPressed(MAN_BTN_NEXT)) go_to(MAN_RED1_GREEN2);
        break;
    case MAN_RED1_GREEN2:
        if (isButtonPressed(MAN_BTN_NEXT)) go_to(MAN_RED1_AMBER2);
        break;
    case MAN_RED1_AMBER2:
        if (isButtonPressed(MAN_BTN_NEXT)) go_to(MAN_ALL_RED_B);
        break;
    case MAN_ALL_RED_B:
        if (isButtonPressed(MAN_BTN_NEXT)) go_to(MAN_GREEN1_RED2);
        break;
    case MAN_GREEN1_RED2:
        if (isButtonPressed(MAN_BTN_NEXT)) go_to(MAN_AMBER1_RED2);
        break;
    case MAN_AMBER1_RED2:
        if (isButtonPressed(MAN_BTN_NEXT)) go_to(MAN_ALL_RED_A);
        break;

    default:
        go_to(MAN_ALL_RED_A);
        break;
    }
}

