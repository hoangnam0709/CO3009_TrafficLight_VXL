/*
 * lcd.h
 *
 *  Created on: Nov 4, 2026
 *      Author: Duy
 */

#ifndef INC_LCD_H_
#define INC_LCD_H_

#include "main.h"
#include "global.h"
#include <stdint.h>

/*
 * LCD 16x2 (HD44780) qua module mở rộng PCF8574, chế độ 4-bit, bus I2C1.
 *
 * Sơ đồ chân PCF8574 trên module LCD chuẩn thị trường:
 *      P0 = RS   P1 = RW   P2 = E   P3 = Backlight
 *      P4 = D4   P5 = D5   P6 = D6  P7 = D7
 *
 * Địa chỉ 7-bit thường gặp: 0x27 (PCF8574T) hoặc 0x3F (PCF8574A).
 * Địa chỉ khác nhau theo từng module -> dò bằng i2c_bus_scan() (bước 3.4) rồi sửa
 * LCD_I2C_ADDRESS bên dưới. Chỉ cần ghi địa chỉ 7-bit, KHÔNG dịch trái.
 */

/* ------------------------------ Cấu hình --------------------------------- */
#define LCD_I2C_ADDRESS     0x27u    /* địa chỉ 7-bit */
#define LCD_I2C_TIMEOUT     20u      /* ms */
#define LCD_COLS            16u
#define LCD_ROWS            2u

#define LCD_BACKLIGHT_ON    0x08u
#define LCD_BACKLIGHT_OFF   0x00u

/* ---------------- Bộ dựng 1 dòng (không đụng phần cứng) ------------------ */
/* Soạn nội dung trong RAM rồi mới đẩy ra LCD 1 lần bằng lcd_fill_line(),
 * nên không phải gọi hàm I2C cho từng ký tự và dòng không đổi sẽ không bị vẽ lại. */
typedef struct {
    char    buf[LCD_COLS + 1u];      /* chuỗi kết thúc bằng '\0' */
    uint8_t len;
} LcdLine_t;

void lcd_line_begin(LcdLine_t *line);
void lcd_line_char(LcdLine_t *line, char c);
void lcd_line_str(LcdLine_t *line, const char *s);
void lcd_line_int(LcdLine_t *line, int32_t num, uint8_t width, char pad);   /* canh phải theo width */
void lcd_line_float(LcdLine_t *line, float value, uint8_t decimals);        /* decimals: 0..3 */

/* --------------------------------- API ----------------------------------- */
void    lcd_init(void);                                  /* gọi 1 lần trong main() (có HAL_Delay) */
uint8_t lcd_is_present(void);                            /* 1 = LCD trả lời trên bus I2C */
uint8_t lcd_is_ready(void);                              /* 1 = đã init và lần ghi gần nhất không lỗi */
void    lcd_backlight(uint8_t on);
void    lcd_clear(void);
void    lcd_goto_XY(uint8_t row, uint8_t col);           /* row: 0..1, col: 0..15 */
void    lcd_putchar(char c);
void    lcd_puts(const char *s);
void    lcd_put_num(int32_t num);                        /* số nguyên có dấu */
void    lcd_put_num_pad(int32_t num, uint8_t width);     /* canh phải, đệm '0' */
void    lcd_put_float(float value, uint8_t decimals);    /* vd 27.35 -> "27.4" (decimals = 1) */
void    lcd_fill_line(uint8_t row, const char *s);       /* in cả dòng, xoá phần dư, bỏ qua nếu nội dung không đổi */
void    lcd_show_line(uint8_t row, const LcdLine_t *line);

#endif /* INC_LCD_H_ */
