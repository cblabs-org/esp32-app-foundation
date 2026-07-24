/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/stubs/platform/lvgl.h
 * Description: Type-complete, behaviorally inert native build stub header for LVGL v9.
 *              Allows LVGLDisplayModule and native unit tests to compile on env:native
 *              without requiring third-party LVGL sources or #ifdef NATIVE_BUILD guards.
 *
 * Note: Later work orders (WO3-WO8) may extend this surface as new LVGL symbols
 *       are introduced within their respective feature boundaries.
 */

#ifndef TEST_STUBS_PLATFORM_LVGL_H_
#define TEST_STUBS_PLATFORM_LVGL_H_

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <cstdarg>

// --- Basic Types & Compatibility ---
typedef int32_t lv_coord_t;
typedef int lv_result_t;

#define LV_RESULT_OK 0
#define LV_RESULT_INVALID -1

// --- Opaque Handle Structs ---
struct lv_display_t {
    void* user_data = nullptr;
    int32_t hor_res = 240;
    int32_t ver_res = 240;
    uint8_t brightness = 255;
    uint8_t rotation = 0;
};

struct lv_indev_t {
    void* user_data = nullptr;
    uint8_t type = 0;
    bool enabled = true;
    struct lv_group_t* group = nullptr;
    struct lv_display_t* disp = nullptr;
};

struct lv_group_t {
    void* user_data = nullptr;
    struct lv_obj_t* focused_obj = nullptr;
};

struct lv_obj_t {
    void* user_data = nullptr;
    struct lv_obj_t* parent = nullptr;
    uint32_t flags = 0;
    int32_t width = 0;
    int32_t height = 0;
    int32_t x = 0;
    int32_t y = 0;
};

struct lv_timer_t {
    void* user_data = nullptr;
};

struct lv_anim_t {
    void* user_data = nullptr;
};

struct lv_event_t {
    void* target = nullptr;
    void* current_target = nullptr;
    uint32_t code = 0;
    void* user_data = nullptr;
    void* param = nullptr;
};

// --- Geometry & Color Structs ---
typedef struct {
    int32_t x;
    int32_t y;
} lv_point_t;

typedef struct {
    int32_t x1;
    int32_t y1;
    int32_t x2;
    int32_t y2;
} lv_area_t;

typedef union {
    uint16_t full;
    struct {
        uint16_t blue : 5;
        uint16_t green : 6;
        uint16_t red : 5;
    } ch;
} lv_color_t;

typedef struct {
    struct {
        uint32_t cf : 8;
        uint32_t always_zero : 3;
        uint32_t reserved : 2;
        uint32_t w : 11;
        uint32_t h : 8;
    } header;
    uint32_t data_size;
    const uint8_t* data;
    const void* reserved;
} lv_image_dsc_t;

typedef lv_image_dsc_t lv_img_dsc_t;

// --- Enums & Constants ---
typedef enum {
    LV_COLOR_FORMAT_UNKNOWN = 0,
    LV_COLOR_FORMAT_RAW = 0x01,
    LV_COLOR_FORMAT_RAW_ALPHA = 0x02,
    LV_COLOR_FORMAT_L8 = 0x06,
    LV_COLOR_FORMAT_I1 = 0x07,
    LV_COLOR_FORMAT_I2 = 0x08,
    LV_COLOR_FORMAT_I4 = 0x09,
    LV_COLOR_FORMAT_I8 = 0x0A,
    LV_COLOR_FORMAT_A8 = 0x0E,
    LV_COLOR_FORMAT_RGB565 = 0x12,
    LV_COLOR_FORMAT_RGB888 = 0x13,
    LV_COLOR_FORMAT_XRGB8888 = 0x14,
    LV_COLOR_FORMAT_ARGB8888 = 0x15,
    LV_COLOR_FORMAT_NATIVE = LV_COLOR_FORMAT_RGB565
} lv_color_format_t;

typedef enum {
    LV_DISPLAY_RENDER_MODE_PARTIAL = 0,
    LV_DISPLAY_RENDER_MODE_DIRECT = 1,
    LV_DISPLAY_RENDER_MODE_FULL = 2
} lv_display_render_mode_t;

typedef enum {
    LV_DISPLAY_ROTATION_0 = 0,
    LV_DISPLAY_ROTATION_90 = 1,
    LV_DISPLAY_ROTATION_180 = 2,
    LV_DISPLAY_ROTATION_270 = 3
} lv_display_rotation_t;

typedef enum {
    LV_INDEV_TYPE_NONE = 0,
    LV_INDEV_TYPE_POINTER,
    LV_INDEV_TYPE_KEYPAD,
    LV_INDEV_TYPE_BUTTON,
    LV_INDEV_TYPE_ENCODER
} lv_indev_type_t;

typedef enum {
    LV_INDEV_STATE_RELEASED = 0,
    LV_INDEV_STATE_PRESSED = 1
} lv_indev_state_t;

typedef struct {
    lv_point_t point;
    int16_t enc_diff;
    uint32_t key;
    uint32_t btn_id;
    lv_indev_state_t state;
    bool continue_reading;
} lv_indev_data_t;

typedef enum {
    LV_SCR_LOAD_ANIM_NONE = 0,
    LV_SCR_LOAD_ANIM_OVER_LEFT,
    LV_SCR_LOAD_ANIM_OVER_RIGHT,
    LV_SCR_LOAD_ANIM_OVER_TOP,
    LV_SCR_LOAD_ANIM_OVER_BOTTOM,
    LV_SCR_LOAD_ANIM_MOVE_LEFT,
    LV_SCR_LOAD_ANIM_MOVE_RIGHT,
    LV_SCR_LOAD_ANIM_MOVE_TOP,
    LV_SCR_LOAD_ANIM_MOVE_BOTTOM,
    LV_SCR_LOAD_ANIM_FADE_IN,
    LV_SCR_LOAD_ANIM_FADE_ON,
    LV_SCR_LOAD_ANIM_FADE_OUT,
    LV_SCR_LOAD_ANIM_OUT_LEFT,
    LV_SCR_LOAD_ANIM_OUT_RIGHT,
    LV_SCR_LOAD_ANIM_OUT_TOP,
    LV_SCR_LOAD_ANIM_OUT_BOTTOM
} lv_screen_load_anim_t;

typedef lv_screen_load_anim_t lv_scr_load_anim_t;

// Alignments
#define LV_ALIGN_DEFAULT          0
#define LV_ALIGN_TOP_LEFT         1
#define LV_ALIGN_TOP_MID          2
#define LV_ALIGN_TOP_RIGHT        3
#define LV_ALIGN_BOTTOM_LEFT      4
#define LV_ALIGN_BOTTOM_MID       5
#define LV_ALIGN_BOTTOM_RIGHT     6
#define LV_ALIGN_LEFT_MID         7
#define LV_ALIGN_RIGHT_MID        8
#define LV_ALIGN_CENTER           9
#define LV_ALIGN_OUT_TOP_LEFT     10
#define LV_ALIGN_OUT_TOP_MID      11
#define LV_ALIGN_OUT_TOP_RIGHT    12
#define LV_ALIGN_OUT_BOTTOM_LEFT  13
#define LV_ALIGN_OUT_BOTTOM_MID   14
#define LV_ALIGN_OUT_BOTTOM_RIGHT 15
#define LV_ALIGN_OUT_LEFT_TOP     16
#define LV_ALIGN_OUT_LEFT_MID     17
#define LV_ALIGN_OUT_LEFT_BOTTOM  18
#define LV_ALIGN_OUT_RIGHT_TOP    19
#define LV_ALIGN_OUT_RIGHT_MID   20
#define LV_ALIGN_OUT_RIGHT_BOTTOM 21

// Flags
#define LV_OBJ_FLAG_HIDDEN           (1L << 0)
#define LV_OBJ_FLAG_CLICKABLE        (1L << 1)
#define LV_OBJ_FLAG_CLICK_FOCUSABLE  (1L << 2)
#define LV_OBJ_FLAG_CHECKABLE        (1L << 3)
#define LV_OBJ_FLAG_SCROLLABLE       (1L << 4)
#define LV_OBJ_FLAG_SCROLL_ELASTIC   (1L << 5)
#define LV_OBJ_FLAG_SCROLL_MOMENTUM  (1L << 6)
#define LV_OBJ_FLAG_SCROLL_ONE       (1L << 7)

// Label long modes
#define LV_LABEL_LONG_WRAP            0
#define LV_LABEL_LONG_DOT             1
#define LV_LABEL_LONG_SCROLL          2
#define LV_LABEL_LONG_SCROLL_CIRCULAR 3
#define LV_LABEL_LONG_CLIP            4

// Event Codes
typedef enum {
    LV_EVENT_ALL = 0,
    LV_EVENT_PRESSED,
    LV_EVENT_PRESSING,
    LV_EVENT_PRESS_LOST,
    LV_EVENT_SHORT_CLICKED,
    LV_EVENT_LONG_CLICKED,
    LV_EVENT_LONG_CLICKED_REPEAT,
    LV_EVENT_CLICKED,
    LV_EVENT_RELEASED,
    LV_EVENT_SCROLL_BEGIN,
    LV_EVENT_SCROLL_END,
    LV_EVENT_SCROLL,
    LV_EVENT_GESTURE,
    LV_EVENT_KEY,
    LV_EVENT_FOCUSED,
    LV_EVENT_DEFOCUSED,
    LV_EVENT_LEAVE,
    LV_EVENT_VALUE_CHANGED,
    LV_EVENT_INSERT,
    LV_EVENT_REFRESH,
    LV_EVENT_READY,
    LV_EVENT_CANCEL,
    LV_EVENT_DELETE
} lv_event_code_t;

// Styles, Parts, States
#define LV_PART_MAIN         0x000000
#define LV_PART_SCROLLBAR    0x010000
#define LV_PART_INDICATOR    0x020000
#define LV_PART_KNOB         0x030000
#define LV_PART_SELECTED     0x040000
#define LV_PART_ITEMS        0x050000
#define LV_PART_CURSOR       0x060000
#define LV_PART_CUSTOM_FIRST 0x080000
#define LV_PART_ANY          0x0F0000

#define LV_STATE_DEFAULT     0x0000
#define LV_STATE_CHECKED     0x0001
#define LV_STATE_FOCUSED     0x0002
#define LV_STATE_FOCUS_KEY   0x0004
#define LV_STATE_EDITED      0x0008
#define LV_STATE_HOVERED     0x0010
#define LV_STATE_PRESSED     0x0020
#define LV_STATE_SCROLLED    0x0040
#define LV_STATE_DISABLED    0x0080

// Opacity
#define LV_OPA_TRANSP 0
#define LV_OPA_0      0
#define LV_OPA_10     25
#define LV_OPA_20     51
#define LV_OPA_30     76
#define LV_OPA_40     102
#define LV_OPA_50     127
#define LV_OPA_60     153
#define LV_OPA_70     178
#define LV_OPA_80     204
#define LV_OPA_90     229
#define LV_OPA_100    255
#define LV_OPA_COVER  255

// --- Callback Function Signatures ---
typedef void (*lv_display_flush_cb_t)(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);
typedef void (*lv_indev_read_cb_t)(lv_indev_t* indev, lv_indev_data_t* data);
typedef void (*lv_event_cb_t)(lv_event_t* e);

// --- Fonts ---
typedef struct _lv_font_t {
    void* dsc;
} lv_font_t;

inline static const lv_font_t lv_font_montserrat_14 = { nullptr };
inline static const lv_font_t lv_font_montserrat_20 = { nullptr };
inline static const lv_font_t lv_font_montserrat_28 = { nullptr };
inline static const lv_font_t* lv_font_default = &lv_font_montserrat_14;

// --- Colors ---
inline lv_color_t lv_color_hex(uint32_t c) {
    lv_color_t color;
    color.full = static_cast<uint16_t>(((c >> 8) & 0xF800) | ((c >> 5) & 0x07E0) | ((c >> 3) & 0x001F));
    return color;
}

inline lv_color_t lv_color_make(uint8_t r, uint8_t g, uint8_t b) {
    lv_color_t color;
    color.ch.red = r >> 3;
    color.ch.green = g >> 2;
    color.ch.blue = b >> 3;
    return color;
}

inline lv_color_t lv_color_black(void) {
    return lv_color_hex(0x000000);
}

inline lv_color_t lv_color_white(void) {
    return lv_color_hex(0xFFFFFF);
}

inline uint16_t lv_color_to_u16(lv_color_t color) {
    return color.full;
}

// --- Inline Function Stubs ---

// System Lifecycle
inline void lv_init(void) {}
inline void lv_tick_inc(uint32_t tick_period) { (void)tick_period; }
inline uint32_t lv_timer_handler(void) { return 5; }
inline uint32_t lv_tick_get(void) { return 0; }

// Display Management
inline lv_display_t* lv_display_create(int32_t hor_res, int32_t ver_res) {
    lv_display_t* disp = new lv_display_t();
    disp->hor_res = hor_res;
    disp->ver_res = ver_res;
    return disp;
}

inline void lv_display_delete(lv_display_t* disp) {
    delete disp;
}

inline void lv_display_set_buffers(lv_display_t* disp, void* buf1, void* buf2, uint32_t buf_size, lv_display_render_mode_t render_mode) {
    (void)disp; (void)buf1; (void)buf2; (void)buf_size; (void)render_mode;
}

inline void lv_display_set_flush_cb(lv_display_t* disp, lv_display_flush_cb_t flush_cb) {
    (void)disp; (void)flush_cb;
}

inline void lv_display_flush_ready(lv_display_t* disp) {
    (void)disp;
}

inline void lv_display_set_brightness(lv_display_t* disp, uint8_t level) {
    if (disp) disp->brightness = level;
}

inline uint8_t lv_display_get_brightness(lv_display_t* disp) {
    return disp ? disp->brightness : 255;
}

inline void lv_display_set_rotation(lv_display_t* disp, lv_display_rotation_t rotation) {
    if (disp) disp->rotation = static_cast<uint8_t>(rotation);
}

inline lv_display_rotation_t lv_display_get_rotation(lv_display_t* disp) {
    return disp ? static_cast<lv_display_rotation_t>(disp->rotation) : LV_DISPLAY_ROTATION_0;
}

inline int32_t lv_display_get_horizontal_resolution(lv_display_t* disp) {
    return disp ? disp->hor_res : 240;
}

inline int32_t lv_display_get_vertical_resolution(lv_display_t* disp) {
    return disp ? disp->ver_res : 240;
}

inline void lv_display_set_user_data(lv_display_t* disp, void* user_data) {
    if (disp) disp->user_data = user_data;
}

inline void* lv_display_get_user_data(lv_display_t* disp) {
    return disp ? disp->user_data : nullptr;
}

inline lv_display_t* lv_display_get_default(void) {
    static lv_display_t default_disp;
    return &default_disp;
}

// Input Device Management
inline lv_indev_t* lv_indev_create(void) {
    return new lv_indev_t();
}

inline void lv_indev_delete(lv_indev_t* indev) {
    delete indev;
}

inline void lv_indev_set_type(lv_indev_t* indev, lv_indev_type_t type) {
    if (indev) indev->type = static_cast<uint8_t>(type);
}

inline void lv_indev_set_read_cb(lv_indev_t* indev, lv_indev_read_cb_t read_cb) {
    (void)indev; (void)read_cb;
}

inline void lv_indev_set_display(lv_indev_t* indev, lv_display_t* disp) {
    if (indev) indev->disp = disp;
}

inline void lv_indev_set_group(lv_indev_t* indev, lv_group_t* group) {
    if (indev) indev->group = group;
}

inline void lv_indev_set_user_data(lv_indev_t* indev, void* user_data) {
    if (indev) indev->user_data = user_data;
}

inline void* lv_indev_get_user_data(lv_indev_t* indev) {
    return indev ? indev->user_data : nullptr;
}

inline void lv_indev_enable(lv_indev_t* indev, bool en) {
    if (indev) indev->enabled = en;
}

// Group Management
inline lv_group_t* lv_group_create(void) {
    return new lv_group_t();
}

inline void lv_group_delete(lv_group_t* group) {
    delete group;
}

inline void lv_group_add_obj(lv_group_t* group, lv_obj_t* obj) {
    (void)group; (void)obj;
}

inline void lv_group_remove_obj(lv_obj_t* obj) {
    (void)obj;
}

inline void lv_group_focus_obj(lv_obj_t* obj) {
    (void)obj;
}

inline lv_obj_t* lv_group_get_focused(const lv_group_t* group) {
    return group ? group->focused_obj : nullptr;
}

// Object / Screen Management
inline lv_obj_t* lv_obj_create(lv_obj_t* parent) {
    lv_obj_t* obj = new lv_obj_t();
    obj->parent = parent;
    return obj;
}

inline void lv_obj_delete(lv_obj_t* obj) {
    delete obj;
}

inline void lv_obj_clean(lv_obj_t* obj) {
    (void)obj;
}

inline void lv_scr_load(lv_obj_t* scr) {
    (void)scr;
}

inline void lv_screen_load(lv_obj_t* scr) {
    (void)scr;
}

inline void lv_scr_load_anim(lv_obj_t* scr, lv_scr_load_anim_t anim_type, uint32_t time, uint32_t delay, bool auto_del) {
    (void)scr; (void)anim_type; (void)time; (void)delay; (void)auto_del;
}

inline void lv_screen_load_anim(lv_obj_t* scr, lv_screen_load_anim_t anim_type, uint32_t time, uint32_t delay, bool auto_del) {
    (void)scr; (void)anim_type; (void)time; (void)delay; (void)auto_del;
}

inline lv_obj_t* lv_screen_active(void) {
    static lv_obj_t active_scr;
    return &active_scr;
}

inline lv_obj_t* lv_scr_act(void) {
    return lv_screen_active();
}

inline void lv_obj_set_size(lv_obj_t* obj, int32_t w, int32_t h) {
    if (obj) { obj->width = w; obj->height = h; }
}

inline void lv_obj_set_width(lv_obj_t* obj, int32_t w) {
    if (obj) obj->width = w;
}

inline void lv_obj_set_height(lv_obj_t* obj, int32_t h) {
    if (obj) obj->height = h;
}

inline void lv_obj_set_pos(lv_obj_t* obj, int32_t x, int32_t y) {
    if (obj) { obj->x = x; obj->y = y; }
}

inline void lv_obj_align(lv_obj_t* obj, uint8_t align, int32_t x_ofs, int32_t y_ofs) {
    (void)align;
    if (obj) { obj->x += x_ofs; obj->y += y_ofs; }
}

inline void lv_obj_center(lv_obj_t* obj) {
    (void)obj;
}

inline void lv_obj_add_flag(lv_obj_t* obj, uint32_t flag) {
    if (obj) obj->flags |= flag;
}

inline void lv_obj_clear_flag(lv_obj_t* obj, uint32_t flag) {
    if (obj) obj->flags &= ~flag;
}

inline bool lv_obj_has_flag(const lv_obj_t* obj, uint32_t flag) {
    return obj ? ((obj->flags & flag) == flag) : false;
}

inline void lv_obj_add_event_cb(lv_obj_t* obj, lv_event_cb_t event_cb, uint32_t filter, void* user_data) {
    (void)obj; (void)event_cb; (void)filter; (void)user_data;
}

inline void lv_obj_set_style_bg_color(lv_obj_t* obj, lv_color_t color, uint32_t selector) {
    (void)obj; (void)color; (void)selector;
}

inline void lv_obj_set_style_bg_opa(lv_obj_t* obj, uint8_t opa, uint32_t selector) {
    (void)obj; (void)opa; (void)selector;
}

inline void lv_obj_set_style_text_color(lv_obj_t* obj, lv_color_t color, uint32_t selector) {
    (void)obj; (void)color; (void)selector;
}

inline void lv_obj_set_style_text_font(lv_obj_t* obj, const lv_font_t* font, uint32_t selector) {
    (void)obj; (void)font; (void)selector;
}

inline void lv_obj_set_style_arc_color(lv_obj_t* obj, lv_color_t color, uint32_t selector) {
    (void)obj; (void)color; (void)selector;
}

inline void lv_obj_set_style_arc_width(lv_obj_t* obj, int32_t width, uint32_t selector) {
    (void)obj; (void)width; (void)selector;
}

// Widgets
inline lv_obj_t* lv_label_create(lv_obj_t* parent) {
    return lv_obj_create(parent);
}

inline void lv_label_set_text(lv_obj_t* label, const char* text) {
    (void)label; (void)text;
}

inline void lv_label_set_text_fmt(lv_obj_t* label, const char* fmt, ...) {
    (void)label; (void)fmt;
}

inline void lv_label_set_long_mode(lv_obj_t* label, uint8_t long_mode) {
    (void)label; (void)long_mode;
}

inline lv_obj_t* lv_arc_create(lv_obj_t* parent) {
    return lv_obj_create(parent);
}

inline void lv_arc_set_value(lv_obj_t* arc, int32_t val) {
    (void)arc; (void)val;
}

inline void lv_arc_set_range(lv_obj_t* arc, int32_t min, int32_t max) {
    (void)arc; (void)min; (void)max;
}

inline void lv_arc_set_bg_angles(lv_obj_t* arc, uint16_t start, uint16_t end) {
    (void)arc; (void)start; (void)end;
}

inline void lv_arc_set_angles(lv_obj_t* arc, uint16_t start, uint16_t end) {
    (void)arc; (void)start; (void)end;
}

inline lv_obj_t* lv_bar_create(lv_obj_t* parent) {
    return lv_obj_create(parent);
}

inline void lv_bar_set_value(lv_obj_t* bar, int32_t val, uint32_t anim) {
    (void)bar; (void)val; (void)anim;
}

inline void lv_bar_set_range(lv_obj_t* bar, int32_t min, int32_t max) {
    (void)bar; (void)min; (void)max;
}

inline lv_obj_t* lv_image_create(lv_obj_t* parent) {
    return lv_obj_create(parent);
}

inline lv_obj_t* lv_img_create(lv_obj_t* parent) {
    return lv_image_create(parent);
}

inline void lv_image_set_src(lv_obj_t* obj, const void* src) {
    (void)obj; (void)src;
}

inline void lv_img_set_src(lv_obj_t* obj, const void* src) {
    lv_image_set_src(obj, src);
}

#endif  // TEST_STUBS_PLATFORM_LVGL_H_
