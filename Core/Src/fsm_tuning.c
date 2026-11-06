/*
 * fsm_tuning.c
 *
 *  Created on: Nov 4, 2026
 *      Author: Duy
 */

#include "fsm_tuning.h"
#include "lcd.h"
#include "button.h"          /* isButtonPressed()  - TV2 */
#include "traffic_light.h"   /* traffic_set()      - TV2 */

/* Khi button.h / traffic_light.h của TV2 chưa có prototype, dùng bản tạm (xem tv3_stubs.h).
 * Sau khi ghép đủ 3 thành viên: thêm symbol TV3_WEAK_STUBS=0 và xoá tv3_stubs.c/.h */
#ifndef TV3_WEAK_STUBS
#define TV3_WEAK_STUBS 1
#endif
#if TV3_WEAK_STUBS
#include "tv3_stubs.h"
#endif

/*
 * Sơ đồ chuyển trạng thái:
 *
 *   [SELECT] --BTN3--> [EDIT] --BTN3 (hợp lệ)--> [SELECT] + lưu vào time_*
 *      |                  |
 *      |                  +--BTN3 (không hợp lệ)--> giữ nguyên [EDIT], LCD báo lỗi
 *      |                  +--BTN4--> [SELECT] + huỷ thay đổi
 *      +--BTN4--> thoát Tuning (status = INIT, 2 đường về Đỏ)
 *
 *   BTN1/BTN2 ở SELECT : đổi màu đang chọn (Đỏ -> Vàng -> Xanh -> Đỏ)
 *   BTN1/BTN2 ở EDIT   : tăng/giảm 1 giây, kẹp biên
 *
 * Không gọi HAL: chỉ dùng traffic_set(), isButtonPressed() (TV2) và các hàm lcd_*.
 */

/* ---------------------------- Biến nội bộ -------------------------------- */
int tune_draft_green = 3;
int tune_draft_amber = 2;
int tune_draft_red   = 5;

static TuneStep_t tune_step   = TUNE_STEP_SELECT;
static uint8_t    tune_sel    = 0u;     /* 0 = Đỏ, 1 = Vàng, 2 = Xanh */
static uint8_t    tune_active = 0u;
static uint8_t    tune_redraw = 1u;
static uint8_t    tune_saved  = 0u;     /* 1 = vừa lưu thành công (hiện thông báo) */
static uint8_t    btn_prev[4] = {0u, 0u, 0u, 0u};   /* mức nút ở tick trước */

/* -------------------------- Hàm trợ giúp nội bộ -------------------------- */

/* Chỉ xử lý tại sườn lên 0 -> 1: đúng cả khi isButtonPressed() của TV2 trả về
 * mức (giữ nút = 1 liên tục) lẫn khi chỉ trả về 1 trong một tick. */
static uint8_t tune_press_edge(uint8_t index)
{
    uint8_t now  = (isButtonPressed(index) != 0u) ? 1u : 0u;
    uint8_t edge = ((now != 0u) && (btn_prev[index] == 0u)) ? 1u : 0u;

    btn_prev[index] = now;
    return edge;
}

static void tune_sync_buttons(void)
{
    uint8_t i;

    for (i = 0u; i < 4u; i++) {
        btn_prev[i] = (isButtonPressed(i) != 0u) ? 1u : 0u;
    }
}

static int tune_clamp(int v, int lo, int hi)
{
    if (v < lo) { return lo; }
    if (v > hi) { return hi; }
    return v;
}

static void tune_load_draft(void)
{
    tune_draft_green = time_green;
    tune_draft_amber = time_amber;
    tune_draft_red   = time_red;
}

/* 1 = bản nháp khác giá trị đang dùng (hiện dấu '*') */
static uint8_t tune_is_dirty(void)
{
    return ((tune_draft_green != time_green) ||
            (tune_draft_amber != time_amber) ||
            (tune_draft_red   != time_red)) ? 1u : 0u;
}

/* 1 = bản nháp thoả toàn bộ ràng buộc */
static uint8_t tune_is_valid(void)
{
    if ((tune_draft_green < TUNE_GREEN_MIN) || (tune_draft_green > TUNE_GREEN_MAX)) { return 0u; }
    if ((tune_draft_amber < TUNE_AMBER_MIN) || (tune_draft_amber > TUNE_AMBER_MAX)) { return 0u; }
    if ((tune_draft_red   < TUNE_RED_MIN)   || (tune_draft_red   > TUNE_RED_MAX))   { return 0u; }
    return (tune_draft_red == (tune_draft_green + tune_draft_amber)) ? 1u : 0u;
}

/* Giữ RED = GREEN + AMBER trên bản nháp: sửa Đỏ thì suy ra Xanh, ngược lại suy ra Đỏ.
 * Sửa Đỏ có thể làm Xanh ra ngoài [3..30] -> tune_is_valid() = 0 và LCD báo lỗi ngay. */
static void tune_recalc(void)
{
    if (tune_sel == 0u) {
        tune_draft_green = tune_draft_red - tune_draft_amber;
    } else {
        tune_draft_red = tune_draft_green + tune_draft_amber;
    }
}

/* Tăng / giảm giá trị của mục đang chọn */
static void tune_step_value(int delta)
{
    switch (tune_sel) {
    case 0u:
        tune_draft_red = tune_clamp(tune_draft_red + delta, TUNE_RED_MIN, TUNE_RED_MAX);
        break;
    case 1u:
        tune_draft_amber = tune_clamp(tune_draft_amber + delta, TUNE_AMBER_MIN, TUNE_AMBER_MAX);
        break;
    default:
        tune_draft_green = tune_clamp(tune_draft_green + delta, TUNE_GREEN_MIN, TUNE_GREEN_MAX);
        break;
    }
    tune_recalc();
}

/* Ghi bản nháp vào biến toàn cục; trả về 1 nếu hợp lệ */
static uint8_t tune_apply(void)
{
    if (tune_is_valid() == 0u) {
        return 0u;
    }
    time_green = tune_draft_green;
    time_amber = tune_draft_amber;
    time_red   = tune_draft_red;
    return 1u;
}

/* Đồng bộ mục đang chọn <-> biến status dùng chung */
static void tune_sel_from_status(void)
{
    if (status == TUNE_RED) {
        tune_sel = 0u;
    } else if (status == TUNE_AMBER) {
        tune_sel = 1u;
    } else {
        tune_sel = 2u;
    }
}

static void tune_status_from_sel(void)
{
    switch (tune_sel) {
    case 0u:  status = TUNE_RED;   break;
    case 1u:  status = TUNE_AMBER; break;
    default:  status = TUNE_GREEN; break;
    }
}

/* Xem trước màu đang chỉnh. Đường 2 luôn Đỏ nên không bao giờ có 2 đường cùng Xanh. */
static void tune_preview(void)
{
    switch (tune_sel) {
    case 0u:
        traffic_set(1u, LIGHT_RED);
        break;
    case 1u:
        traffic_set(1u, LIGHT_AMBER);
        break;
    default:
        traffic_set(1u, LIGHT_GREEN);
        break;
    }
    traffic_set(2u, LIGHT_RED);
}

/* ------------------------------ Hiển thị --------------------------------- */
/* Một mục trên dòng 1: "[G03]" (đang chọn) / "<G03>" (đang sửa) / "G03".
 * Giá trị ngoài [lo..hi] (do sửa Đỏ quá thấp/cao) hiện "??". */
static void tune_line_item(LcdLine_t *l, char tag, int v, int lo, int hi, uint8_t item)
{
    uint8_t selected = (tune_sel == item) ? 1u : 0u;
    uint8_t editing  = (selected != 0u) && (tune_step == TUNE_STEP_EDIT);

    if (selected != 0u) {
        lcd_line_char(l, (editing != 0u) ? '<' : '[');
    }
    lcd_line_char(l, tag);
    if ((v < lo) || (v > hi)) {
        lcd_line_str(l, "??");
    } else {
        lcd_line_int(l, v, 2u, '0');
    }
    if (selected != 0u) {
        lcd_line_char(l, (editing != 0u) ? '>' : ']');
    }
}

void fsm_tuning_display(void)
{
    LcdLine_t l;

    /* Dòng 1: G.. A.. R.. ; cột cuối '*' = có thay đổi chưa lưu */
    lcd_line_begin(&l);
    tune_line_item(&l, 'G', tune_draft_green, TUNE_GREEN_MIN, TUNE_GREEN_MAX, 2u);
    lcd_line_char(&l, ' ');
    tune_line_item(&l, 'A', tune_draft_amber, TUNE_AMBER_MIN, TUNE_AMBER_MAX, 1u);
    lcd_line_char(&l, ' ');
    tune_line_item(&l, 'R', tune_draft_red,   TUNE_RED_MIN,   TUNE_RED_MAX,   0u);
    while (l.len < (LCD_COLS - 1u)) {
        lcd_line_char(&l, ' ');
    }
    lcd_line_char(&l, (tune_is_dirty() != 0u) ? '*' : ' ');
    lcd_show_line(0u, &l);

    /* Dòng 2: hướng dẫn / thông báo */
    if (tune_step == TUNE_STEP_EDIT) {
        if (tune_is_valid() != 0u) {
            lcd_fill_line(1u, "+:1 -:2 OK:3 X:4");
        } else {
            lcd_fill_line(1u, "KHONG HOP LE!   ");
        }
    } else if (tune_saved != 0u) {
        lcd_fill_line(1u, "DA LUU CAU HINH ");
    } else {
        lcd_fill_line(1u, "1<2> 3=SUA 4=OUT");
    }

    tune_redraw = 0u;
}

/* ------------------------------ Vào / thoát ------------------------------ */
void fsm_tuning_enter(void)
{
    tune_active = 1u;
    tune_step   = TUNE_STEP_SELECT;
    tune_saved  = 0u;
    tune_load_draft();
    tune_sync_buttons();       /* tránh ăn nhầm nút đang giữ từ chế độ trước */
    tune_sel_from_status();
    tune_preview();
    fsm_tuning_display();
}

void fsm_tuning_exit(void)
{
    tune_active = 0u;

    /* Trả đèn về trạng thái an toàn: cả 2 đường Đỏ */
    traffic_set(1u, LIGHT_RED);
    traffic_set(2u, LIGHT_RED);

    /* status = INIT để fsm_automatic khởi tạo lại timer từ pha đầu với time_* mới */
    status = INIT;
}

uint8_t fsm_tuning_is_active(void)
{
    return tune_active;
}

/* --------------------------------- Task ---------------------------------- */
void fsm_tuning_run(void)
{
    uint8_t in_tune_mode;
    uint8_t e_inc;
    uint8_t e_dec;
    uint8_t e_ok;
    uint8_t e_exit;

    in_tune_mode = (uint8_t)((status == TUNE_RED) ||
                             (status == TUNE_AMBER) ||
                             (status == TUNE_GREEN));

    /* Module khác vừa gán status = TUNE_* -> bắt đầu phiên tinh chỉnh */
    if ((tune_active == 0u) && (in_tune_mode != 0u)) {
        fsm_tuning_enter();
        return;
    }
    if (tune_active == 0u) {
        return;
    }
    /* Chế độ bị đổi từ bên ngoài -> nhường quyền điều khiển */
    if (in_tune_mode == 0u) {
        tune_active = 0u;
        return;
    }

    /* Có module khác đặt thẳng status = TUNE_AMBER / TUNE_GREEN */
    {
        uint8_t prev_sel = tune_sel;

        tune_sel_from_status();
        if (prev_sel != tune_sel) {
            tune_preview();
            tune_redraw = 1u;
        }
    }

    /* Đọc đủ 4 nút mỗi tick để giữ đúng lịch sử mức nút */
    e_inc  = tune_press_edge(TUNE_BTN_INC);
    e_dec  = tune_press_edge(TUNE_BTN_DEC);
    e_ok   = tune_press_edge(TUNE_BTN_OK);
    e_exit = tune_press_edge(TUNE_BTN_EXIT);

    if (tune_step == TUNE_STEP_SELECT) {
        /* --- Bước 1: chọn màu cần chỉnh --- */
        if (e_inc != 0u) {
            tune_sel = (uint8_t)((tune_sel + 1u) % 3u);
            tune_status_from_sel();
            tune_saved  = 0u;
            tune_redraw = 1u;
            tune_preview();
        }
        if (e_dec != 0u) {
            tune_sel = (uint8_t)((tune_sel + 2u) % 3u);
            tune_status_from_sel();
            tune_saved  = 0u;
            tune_redraw = 1u;
            tune_preview();
        }
        if (e_ok != 0u) {
            tune_step   = TUNE_STEP_EDIT;
            tune_saved  = 0u;
            tune_redraw = 1u;
        }
        if (e_exit != 0u) {
            fsm_tuning_exit();
            return;
        }
    } else {
        /* --- Bước 2: tăng / giảm giá trị, lưu hoặc huỷ --- */
        if (e_inc != 0u) {
            tune_step_value(1);
            tune_redraw = 1u;
        }
        if (e_dec != 0u) {
            tune_step_value(-1);
            tune_redraw = 1u;
        }
        if (e_ok != 0u) {
            if (tune_apply() != 0u) {          /* hợp lệ: lưu và quay về chọn màu */
                tune_step  = TUNE_STEP_SELECT;
                tune_saved = 1u;
            }                                  /* không hợp lệ: ở lại EDIT, LCD đã báo lỗi */
            tune_redraw = 1u;
        }
        if (e_exit != 0u) {
            tune_load_draft();                 /* huỷ toàn bộ thay đổi nháp */
            tune_step   = TUNE_STEP_SELECT;
            tune_saved  = 0u;
            tune_redraw = 1u;
        }
    }

    if (tune_redraw != 0u) {
        fsm_tuning_display();
    }
}
