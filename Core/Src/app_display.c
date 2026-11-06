/*
 * app_display.c
 *
 *  Created on: Nov 4, 2026
 *      Author: Duy
 */

#include "app_display.h"
#include "lcd.h"
#include "dht20.h"

/* Khi LCD mất kết nối, thử khởi tạo lại sau mỗi 10 lần gọi = 2 s */
#define APP_DISPLAY_RETRY_CALLS     10u

static uint8_t retry_count = 0u;

void app_display_init(void)
{
    lcd_init();

    if (lcd_is_ready() != 0u) {
        lcd_fill_line(0u, "CO3009 TRAFFIC");
        lcd_fill_line(1u, "LCD OK - DHT20");
    }
    /* Không thấy LCD: hệ thống vẫn chạy bình thường. Dò lại địa chỉ bằng
     * i2c_bus_scan() rồi sửa LCD_I2C_ADDRESS trong lcd.h. */
}

/* Tên màu 3 ký tự cho dòng chế độ thủ công */
static const char *light_name(uint8_t color)
{
    switch (color) {
    case LIGHT_RED:    return "RED";
    case LIGHT_AMBER:  return "AMB";
    case LIGHT_GREEN:  return "GRN";
    default:           return "OFF";
    }
}

static void display_env_line(void)
{
    LcdLine_t l;

    lcd_line_begin(&l);
    lcd_line_str(&l, "T:");
    if (dht20_is_valid() != 0u) {
        lcd_line_float(&l, temperature, 1u);
    } else {
        lcd_line_str(&l, "--.-");
    }
    lcd_line_str(&l, "C H:");
    if (dht20_is_valid() != 0u) {
        lcd_line_float(&l, humidity, 0u);
    } else {
        lcd_line_str(&l, "--");
    }
    lcd_line_char(&l, '%');
    lcd_show_line(0u, &l);
}

static void display_mode_line(void)
{
    LcdLine_t l;
    uint8_t   c1 = LIGHT_OFF;
    uint8_t   c2 = LIGHT_OFF;

    lcd_line_begin(&l);

    switch (status) {
    case AUTO_RED_GREEN:
    case AUTO_RED_AMBER:
    case AUTO_GREEN_RED:
    case AUTO_AMBER_RED:
        lcd_line_str(&l, "AUTO R1:");
        lcd_line_int(&l, road1_countdown, 2u, '0');
        lcd_line_str(&l, " R2:");
        lcd_line_int(&l, road2_countdown, 2u, '0');
        break;

    case MAN_RED_GREEN:   c1 = LIGHT_RED;   c2 = LIGHT_GREEN; break;
    case MAN_RED_AMBER:   c1 = LIGHT_RED;   c2 = LIGHT_AMBER; break;
    case MAN_GREEN_RED:   c1 = LIGHT_GREEN; c2 = LIGHT_RED;   break;
    case MAN_AMBER_RED:   c1 = LIGHT_AMBER; c2 = LIGHT_RED;   break;

    case INIT:
        lcd_line_str(&l, "INIT ...");
        break;

    default:
        lcd_line_str(&l, "CO3009 TRAFFIC");
        break;
    }

    if ((status >= MAN_RED_GREEN) && (status <= MAN_AMBER_RED)) {
        lcd_line_str(&l, "MAN 1:");
        lcd_line_str(&l, light_name(c1));
        lcd_line_str(&l, " 2:");
        lcd_line_str(&l, light_name(c2));
    }

    lcd_show_line(1u, &l);
}

void app_display_run(void)
{
    /* Chế độ Tuning: fsm_tuning tự vẽ màn hình của nó */
    if ((status == TUNE_RED) || (status == TUNE_AMBER) || (status == TUNE_GREEN)) {
        return;
    }

    if (lcd_is_ready() == 0u) {
        retry_count++;
        if (retry_count >= APP_DISPLAY_RETRY_CALLS) {
            retry_count = 0u;
            lcd_init();                    /* blocking ~70 ms nhưng chỉ khi LCD vừa được cắm lại */
        }
        return;
    }
    retry_count = 0u;

    display_env_line();
    display_mode_line();
}
