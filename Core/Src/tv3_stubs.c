/*
 * tv3_stubs.c
 *
 *  Created on: Nov 4, 2026
 *      Author: Duy
 */

#include "tv3_stubs.h"

#ifndef TV3_WEAK_STUBS
#define TV3_WEAK_STUBS 1
#endif

#if TV3_WEAK_STUBS
/* Bản weak: nếu TV2 có bản strong thì trình liên kết tự dùng bản của TV2 */
__attribute__((weak)) uint8_t isButtonPressed(uint8_t index)
{
    (void)index;
    return 0u;                  /* chưa có module nút nhấn: coi như không nhấn */
}

__attribute__((weak)) void traffic_set(uint8_t road, uint8_t color)
{
    (void)road;
    (void)color;                /* chưa có module đèn: không xuất GPIO */
}
#endif
