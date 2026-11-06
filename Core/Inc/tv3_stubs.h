/*
 * tv3_stubs.h
 *
 *  Created on: Nov 4, 2026
 *      Author: Duy
 */

#ifndef INC_TV3_STUBS_H_
#define INC_TV3_STUBS_H_

#include <stdint.h>

/*
 * Prototype + hàm tạm (weak) cho 2 hàm của TV2, để nhánh feature/i2c-tuning build được
 * khi button.h / traffic_light.h còn trống. Khi TV2 đã push bản thật:
 *   1. Thêm symbol TV3_WEAK_STUBS=0 (Properties > C/C++ Build > Settings > Preprocessor)
 *   2. Xoá tv3_stubs.h và tv3_stubs.c
 * Chữ ký phải khớp thống nhất với TV2:
 */
uint8_t isButtonPressed(uint8_t index);               /* 0..3 = BTN1..BTN4 */
void    traffic_set(uint8_t road, uint8_t color);     /* road 1..2, color = LIGHT_* trong global.h */

#endif /* INC_TV3_STUBS_H_ */
