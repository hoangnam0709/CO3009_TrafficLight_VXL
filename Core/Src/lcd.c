/*
 * lcd.c
 *
 *  Created on: Nov 4, 2026
 *      Author: Duy
 */

#include "lcd.h"
#include "i2c_bus.h"
#include <string.h>

/*
 * Driver LCD 16x2 qua PCF8574, 4-bit. Một ký tự/lệnh = 4 byte gửi trong MỘT giao dịch I2C:
 *      [hi|E] [hi] [lo|E] [lo]       (E=1 rồi E=0 cho mỗi nibble)
 * Xung E dài bằng thời gian truyền 1 byte I2C (~90 us ở 100 kHz) nên lớn hơn nhiều so với
 * yêu cầu của HD44780 (>= 230 ns) và không phụ thuộc xung nhịp CPU (8 MHz hay 72 MHz đều đúng).
 * Chỉ lệnh Clear / Home cần chờ thêm ~1.6 ms (xem lcd_delay_us()).
 */

/* Các bit điều khiển trên PCF8574 */
#define LCD_RS          0x01u
#define LCD_RW          0x02u   /* luôn = 0: chỉ ghi */
#define LCD_EN          0x04u

/* ---------------------------- Biến nội bộ -------------------------------- */
static uint8_t lcd_bl    = LCD_BACKLIGHT_ON;
static uint8_t lcd_ready = 0u;
static uint8_t lcd_row   = 0u;                       /* hàng của con trỏ hiện tại */
static char    lcd_shadow[LCD_ROWS][LCD_COLS];       /* nội dung đang hiển thị */
static uint8_t lcd_shadow_ok[LCD_ROWS];              /* 1 = lcd_shadow[row] khớp với màn hình */

/* ------------------- Đợi ngắn, tự co giãn theo SystemCoreClock ----------- */
/* Vòng lặp ~ >= 4 chu kỳ CPU/lần nên đợi ít nhất `us` micro giây ở mọi xung nhịp
 * (8 MHz HSI trên F103C6 cũng như 64/72 MHz). Chỉ dùng cho Clear/Home. */
static void lcd_delay_us(uint32_t us)
{
    volatile uint32_t n = us * (SystemCoreClock / 4000000u) + 1u;

    while (n > 0u) {
        n--;
    }
}

/* ------------------------------ Ghi xuống bus ---------------------------- */
static void lcd_bus_write(const uint8_t *buf, uint16_t len)
{
    if (i2c_bus_write(LCD_I2C_ADDRESS, buf, len, LCD_I2C_TIMEOUT) != I2C_BUS_OK) {
        lcd_ready = 0u;           /* LCD không phản hồi: các hàm vẽ sẽ bỏ qua cho đến khi init lại */
    }
}

static void lcd_invalidate(void)
{
    uint8_t r;

    for (r = 0u; r < LCD_ROWS; r++) {
        lcd_shadow_ok[r] = 0u;
    }
}

/* Chốt 1 nibble (dùng riêng cho chuỗi khởi tạo) */
static void lcd_write_nibble(uint8_t nib)
{
    uint8_t d = (uint8_t)(((nib & 0x0Fu) << 4) | lcd_bl);
    uint8_t buf[2];

    buf[0] = (uint8_t)(d | LCD_EN);
    buf[1] = d;
    lcd_bus_write(buf, 2u);
}

/* Ghi 1 byte lệnh (rs = 0) hoặc dữ liệu (rs = 1) */
static void lcd_write_byte(uint8_t value, uint8_t rs)
{
    uint8_t ctl = (uint8_t)(lcd_bl | ((rs != 0u) ? LCD_RS : 0u));
    uint8_t hi  = (uint8_t)((value & 0xF0u) | ctl);
    uint8_t lo  = (uint8_t)(((uint8_t)(value << 4) & 0xF0u) | ctl);
    uint8_t buf[4];

    buf[0] = (uint8_t)(hi | LCD_EN);
    buf[1] = hi;
    buf[2] = (uint8_t)(lo | LCD_EN);
    buf[3] = lo;
    lcd_bus_write(buf, 4u);
}

static void lcd_cmd(uint8_t cmd)
{
    lcd_write_byte(cmd, 0u);
    if ((cmd == 0x01u) || (cmd == 0x02u)) {
        lcd_delay_us(2000u);       /* Clear / Return home cần ~1.52 ms */
    }
}

static void lcd_data(uint8_t data)
{
    lcd_write_byte(data, 1u);
}

/* ------------------------------ Khởi tạo --------------------------------- */
void lcd_init(void)
{
    lcd_bl = LCD_BACKLIGHT_ON;
    lcd_invalidate();
    lcd_row = 0u;

    if (i2c_bus_probe(LCD_I2C_ADDRESS) != I2C_BUS_OK) {
        lcd_ready = 0u;
        return;
    }
    lcd_ready = 1u;

    HAL_Delay(50);                         /* chờ nguồn LCD ổn định (chỉ chạy lúc khởi động) */

    /* Trình tự khởi tạo 4-bit theo datasheet HD44780 */
    lcd_write_nibble(0x03u); HAL_Delay(5);
    lcd_write_nibble(0x03u); HAL_Delay(2);
    lcd_write_nibble(0x03u); HAL_Delay(2);
    lcd_write_nibble(0x02u); HAL_Delay(2); /* chuyển sang 4-bit */

    lcd_cmd(0x28u);   /* 4-bit, 2 dòng, font 5x8 */
    lcd_cmd(0x0Cu);   /* bật hiển thị, tắt con trỏ */
    lcd_cmd(0x06u);   /* tự tăng địa chỉ, không dịch màn hình */
    lcd_cmd(0x01u);   /* xoá màn hình */
}

uint8_t lcd_is_present(void)
{
    return (i2c_bus_probe(LCD_I2C_ADDRESS) == I2C_BUS_OK) ? 1u : 0u;
}

uint8_t lcd_is_ready(void)
{
    return lcd_ready;
}

void lcd_backlight(uint8_t on)
{
    uint8_t v;

    lcd_bl = (on != 0u) ? LCD_BACKLIGHT_ON : LCD_BACKLIGHT_OFF;
    v = lcd_bl;
    if (lcd_ready != 0u) {
        lcd_bus_write(&v, 1u);
    }
}

/* ------------------------------ Hiển thị --------------------------------- */
void lcd_clear(void)
{
    if (lcd_ready == 0u) {
        return;
    }
    lcd_cmd(0x01u);
    lcd_invalidate();
    lcd_row = 0u;
}

void lcd_goto_XY(uint8_t row, uint8_t col)
{
    uint8_t addr;

    if (lcd_ready == 0u) {
        return;
    }
    if (row >= LCD_ROWS) { row = 0u; }
    if (col >= LCD_COLS) { col = (uint8_t)(LCD_COLS - 1u); }

    addr = (row == 0u) ? col : (uint8_t)(0x40u + col);
    lcd_cmd((uint8_t)(0x80u | addr));     /* Set DDRAM address */
    lcd_row = row;
}

/* Ghi trực tiếp tại con trỏ -> nội dung dòng đó không còn khớp với bản nhớ đệm */
void lcd_putchar(char c)
{
    if (lcd_ready == 0u) {
        return;
    }
    lcd_shadow_ok[lcd_row] = 0u;
    lcd_data((uint8_t)c);
}

void lcd_puts(const char *s)
{
    while ((s != 0) && (*s != '\0')) {
        lcd_putchar(*s);
        s++;
    }
}

/* --------------------------- Định dạng số (RAM) -------------------------- */
/* Ghi vào dst (>= 24 byte), trả về độ dài. width tính cả dấu '-'. */
static uint8_t lcd_fmt_int(char *dst, int32_t num, uint8_t width, char pad)
{
    char     tmp[11];
    uint8_t  n = 0u;
    uint8_t  len = 0u;
    uint8_t  neg = (num < 0) ? 1u : 0u;
    uint8_t  total;
    uint32_t u = (neg != 0u) ? ((uint32_t)0u - (uint32_t)num) : (uint32_t)num;

    do {
        tmp[n++] = (char)('0' + (u % 10u));
        u /= 10u;
    } while ((u > 0u) && (n < 10u));

    total = (uint8_t)(n + neg);

    if (pad == '0') {                                   /* -007 */
        if (neg != 0u) { dst[len++] = '-'; }
        for (; total < width; total++) { dst[len++] = '0'; }
    } else {                                            /*   -7 */
        for (; total < width; total++) { dst[len++] = pad; }
        if (neg != 0u) { dst[len++] = '-'; }
    }
    while (n > 0u) {
        dst[len++] = tmp[--n];
    }
    dst[len] = '\0';
    return len;
}

static uint8_t lcd_fmt_float(char *dst, float value, uint8_t decimals)
{
    int32_t scale = 1;
    int32_t scaled;
    int32_t mag;
    uint8_t i;
    uint8_t len = 0u;

    if (decimals > 3u) { decimals = 3u; }
    for (i = 0u; i < decimals; i++) { scale *= 10; }

    if (value >  999999.0f) { value =  999999.0f; }     /* tránh tràn khi ép kiểu */
    if (value < -999999.0f) { value = -999999.0f; }

    scaled = (int32_t)((value * (float)scale) + ((value >= 0.0f) ? 0.5f : -0.5f));
    if (scaled < 0) {
        dst[len++] = '-';
        mag = -scaled;
    } else {
        mag = scaled;
    }

    len = (uint8_t)(len + lcd_fmt_int(&dst[len], mag / scale, 0u, ' '));
    if (decimals > 0u) {
        dst[len++] = '.';
        len = (uint8_t)(len + lcd_fmt_int(&dst[len], mag % scale, decimals, '0'));
    }
    dst[len] = '\0';
    return len;
}

void lcd_put_num(int32_t num)
{
    char t[24];

    (void)lcd_fmt_int(t, num, 0u, ' ');
    lcd_puts(t);
}

void lcd_put_num_pad(int32_t num, uint8_t width)
{
    char t[24];

    if (width > LCD_COLS) { width = LCD_COLS; }
    (void)lcd_fmt_int(t, num, width, '0');
    lcd_puts(t);
}

void lcd_put_float(float value, uint8_t decimals)
{
    char t[24];

    (void)lcd_fmt_float(t, value, decimals);
    lcd_puts(t);
}

/* ---------------------------- Bộ dựng 1 dòng ----------------------------- */
void lcd_line_begin(LcdLine_t *line)
{
    line->len = 0u;
    line->buf[0] = '\0';
}

void lcd_line_char(LcdLine_t *line, char c)
{
    if (line->len < LCD_COLS) {
        line->buf[line->len] = c;
        line->len++;
        line->buf[line->len] = '\0';
    }
}

void lcd_line_str(LcdLine_t *line, const char *s)
{
    while ((s != 0) && (*s != '\0')) {
        lcd_line_char(line, *s);
        s++;
    }
}

void lcd_line_int(LcdLine_t *line, int32_t num, uint8_t width, char pad)
{
    char t[24];

    if (width > LCD_COLS) { width = LCD_COLS; }
    (void)lcd_fmt_int(t, num, width, pad);
    lcd_line_str(line, t);
}

void lcd_line_float(LcdLine_t *line, float value, uint8_t decimals)
{
    char t[24];

    (void)lcd_fmt_float(t, value, decimals);
    lcd_line_str(line, t);
}

/* ------------------------------ In cả dòng ------------------------------- */
void lcd_fill_line(uint8_t row, const char *s)
{
    char    text[LCD_COLS];
    uint8_t i;

    if ((lcd_ready == 0u) || (row >= LCD_ROWS)) {
        return;
    }

    for (i = 0u; i < LCD_COLS; i++) {                   /* cắt / đệm khoảng trắng đủ 16 ký tự */
        if ((s != 0) && (*s != '\0')) {
            text[i] = *s;
            s++;
        } else {
            text[i] = ' ';
        }
    }

    if ((lcd_shadow_ok[row] != 0u) && (memcmp(text, lcd_shadow[row], LCD_COLS) == 0)) {
        return;                                         /* nội dung không đổi: khỏi vẽ lại */
    }

    lcd_goto_XY(row, 0u);
    for (i = 0u; i < LCD_COLS; i++) {
        lcd_data((uint8_t)text[i]);
    }
    if (lcd_ready != 0u) {                              /* chỉ ghi nhớ nếu các lần ghi đều thành công */
        memcpy(lcd_shadow[row], text, LCD_COLS);
        lcd_shadow_ok[row] = 1u;
    }
}

void lcd_show_line(uint8_t row, const LcdLine_t *line)
{
    lcd_fill_line(row, line->buf);
}
