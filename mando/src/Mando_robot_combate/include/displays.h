/* displays.h */

#ifndef DISPLAYS_H
#define DISPLAYS_H

#include <lvgl.h>
#include <lvgl_helpers.h>

/* ---------- SPLASH ---------- */
lv_obj_t *screen_splash;
lv_obj_t *img_splash;

LV_IMG_DECLARE(splash_logo);

/* ---------- WARNING ---------- */
lv_obj_t *screen_warning;
lv_obj_t *label_warning_top;
lv_obj_t *label_warning_center;

/* ---------- HOME ---------- */
lv_obj_t *home;
lv_obj_t *label_home;
lv_obj_t *label_home_center;
lv_style_t style_arc_panel;
lv_style_t style_arc_motor;
lv_obj_t *arc_panel;
lv_obj_t *arc_motor;
lv_anim_t anim_arc;

/* ---------- HOME CHART ---------- */
lv_obj_t *home_chart;
lv_obj_t *label_home_chart;
lv_obj_t *chart_home_chart;
lv_chart_series_t *ser1;
lv_chart_series_t *ser2;
lv_obj_t *label_chart_home_chart_right;

/* ---------- PANEL ---------- */
lv_obj_t *screen_panel;
lv_obj_t *label_panel;
lv_obj_t *label_panel_center;
lv_obj_t *meter_panel;
lv_meter_indicator_t *indic_meter_panel;
lv_anim_t anim_panel;

/* ---------- MOTOR ---------- */
lv_obj_t *screen_motor;
lv_obj_t *label_motor;
lv_obj_t *label_motor_center;
lv_obj_t *meter_motor;
lv_meter_indicator_t *indic_meter_motor;
lv_anim_t anim_motor;

/* ---------- BATERIA ---------- */
lv_obj_t *screen_bateria;
lv_obj_t *label_bateria;
lv_obj_t *label_bateria_left;
lv_obj_t *label_bateria_right;
lv_obj_t *bar_bateria;
lv_style_t style_bar_bateria;
lv_style_t style_bar_bateria_border;
lv_anim_t anim_bateria;

/* ---------- GPS ---------- */
lv_obj_t *screen_gps;
lv_obj_t *label_gps;
lv_obj_t *label_gps_center;
lv_obj_t *meter_gps;
lv_meter_scale_t *scale_gps;
lv_anim_t anim_gps;

/* ---------- RELOJ ---------- */
lv_obj_t *screen_reloj;
lv_obj_t *label_reloj;
lv_obj_t *label_reloj_center;
lv_obj_t *label_reloj_top;

/* ---------- CALIBRATE ---------- */
lv_obj_t *screen_calibrate;
lv_obj_t *label_calibrate;
lv_obj_t *label_calibrate_center;

/* ---------- MENU ---------- */
lv_obj_t *screen_menu;
lv_obj_t *label_menu;
lv_obj_t *label_menu_center;

/* ---------- MENU CONFIG ---------- */
lv_obj_t *screen_menu_config;
lv_obj_t *list_menu_config;

/* =====================================================================
 * Callbacks de actualización
 * ===================================================================== */

static void set_angle_arc_panel(void *obj, int32_t v)
{
    lv_arc_set_value(arc_panel, v);
}

static void set_angle_arc_motor(void *obj, int32_t v)
{
    lv_arc_set_value(arc_motor, v);
}

static void change_home(uint8_t motor_percent, int8_t right_y)
{
    lv_anim_set_values(&anim_arc, lv_arc_get_value(arc_motor), (int32_t)motor_percent);
    lv_anim_set_exec_cb(&anim_arc, set_angle_arc_motor);
    lv_anim_start(&anim_arc);

    uint8_t abs_val = (right_y >= 0) ? (uint8_t)right_y
                                     : (uint8_t)(-right_y);
    lv_color_t color = (right_y >= 0)
                       ? lv_palette_main(LV_PALETTE_GREEN)
                       : lv_palette_main(LV_PALETTE_RED);
    lv_obj_set_style_arc_color(arc_panel, color, LV_PART_INDICATOR);

    lv_anim_set_values(&anim_arc, lv_arc_get_value(arc_panel), (int32_t)abs_val);
    lv_anim_set_exec_cb(&anim_arc, set_angle_arc_panel);
    lv_anim_start(&anim_arc);

    lv_label_set_text_fmt(label_home_center, "MOTOR\n%u%%", (unsigned)motor_percent);
}

static void change_home_chart(uint8_t val_motor, uint8_t val_panel)
{
    lv_chart_set_next_value(chart_home_chart, ser1, val_panel);
    lv_chart_set_next_value(chart_home_chart, ser2, val_motor);
}

static void set_value_panel(void *indic, int32_t v)
{
    lv_meter_set_indicator_end_value(meter_panel, indic_meter_panel, v);
}

static void change_screen_panel(float v, float i)
{
    float x = v * i;
    lv_anim_set_values(&anim_panel, indic_meter_panel->end_value, (int32_t)x);
    lv_anim_set_exec_cb(&anim_panel, set_value_panel);
    lv_anim_start(&anim_panel);
    lv_label_set_text_fmt(label_panel_center, "%.1f V\n%.1f A\n%.1f W", v, i, x);
}

static void set_value_motor(void *indic, int32_t v)
{
    lv_meter_set_indicator_end_value(meter_motor, indic_meter_motor, v);
}

static void change_screen_motor(float v, float i)
{
    float x = v * i;
    lv_anim_set_values(&anim_motor, indic_meter_motor->end_value, (int32_t)x);
    lv_anim_set_exec_cb(&anim_motor, set_value_motor);
    lv_anim_start(&anim_motor);
    lv_label_set_text_fmt(label_motor_center, "%.1f V\n%.1f A\n%.1f W", v, i, x);
}

static void set_value_bateria(void *obj, int32_t v)
{
    lv_bar_set_value(bar_bateria, v, LV_ANIM_ON);
}

static void change_screen_bateria(float v_mando, float v_barco)
{
    lv_anim_set_values(&anim_bateria, lv_bar_get_value(bar_bateria), (int32_t)(v_mando * 10));
    lv_anim_set_exec_cb(&anim_bateria, set_value_bateria);
    lv_anim_start(&anim_bateria);

    const char *color_barco = "3366ff";
    const char *color_mando = "3366ff";

    if (v_barco >= 8.35f) {
        color_barco = "33ff33";
    } else if (v_barco < 7.4f) {
        color_barco = "ff6633";
    }

    if (v_mando >= 4.15f) {
        color_mando = "33ff33";
        lv_obj_set_style_bg_color(bar_bateria, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
        lv_obj_set_style_border_color(bar_bateria, lv_palette_main(LV_PALETTE_GREEN), 0);
    } else if (v_mando < 3.7f) {
        color_mando = "ff6633";
        lv_obj_set_style_bg_color(bar_bateria, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
        lv_obj_set_style_border_color(bar_bateria, lv_palette_main(LV_PALETTE_RED), 0);
    }

    lv_label_set_text_fmt(label_bateria_left,
                          "#%s " LV_SYMBOL_BATTERY_FULL "# Mando\n#%s " LV_SYMBOL_BATTERY_2 "# Barco",
                          color_mando, color_barco);
    lv_label_set_text_fmt(label_bateria_right, "%.2f V\n%.2f V", v_mando, v_barco);
}

static void set_angle_gps(void *obj, int32_t v)
{
    lv_meter_set_scale_range(meter_gps, scale_gps, 0, 60, 360, v);
}

static void change_screen_gps(float latitud, float longitud)
{
    lv_label_set_text_fmt(label_gps_center,
                          "Latitud\n%f\nLongitud\n%f",
                          latitud, longitud);
}

static void change_screen_reloj(uint8_t hour, uint8_t min,
                                uint8_t year, uint8_t month, uint8_t day)
{
    static const char *months[12] = {
        "Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio",
        "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"
    };

    lv_label_set_text_fmt(label_reloj_center, "%02u : %02u", hour, min);

    if (month >= 1 && month <= 12) {
        lv_label_set_text_fmt(label_reloj_top, "%02u %s 20%02u",
                              day, months[month - 1], year);
    }
}

/* =====================================================================
 * Construcción de pantallas
 * ===================================================================== */

static void display_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED) {
        const char *txt = lv_list_get_btn_text(list_menu_config, obj);
        LV_LOG_USER("Clicked: %s", txt ? txt : "?");
    }
}

static void ui_build_screens(void)
{
/* ================= SPLASH ================= */
    screen_splash = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_splash, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen_splash, LV_OPA_100, 0);

    img_splash = lv_img_create(screen_splash);
    lv_img_set_src(img_splash, &splash_logo);
    lv_obj_center(img_splash);

/* ================= WARNING ================= */
    screen_warning = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_warning, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen_warning, LV_OPA_100, 0);

    label_warning_top = lv_label_create(screen_warning);
    lv_obj_set_align(label_warning_top, LV_ALIGN_TOP_MID);
    lv_obj_set_y(label_warning_top, 30);
    lv_obj_set_style_text_font(label_warning_top, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(label_warning_top, lv_palette_main(LV_PALETTE_RED), 0);
    lv_label_set_text(label_warning_top, "! ATENCION !");

    label_warning_center = lv_label_create(screen_warning);
    lv_obj_center(label_warning_center);
    lv_obj_set_style_text_font(label_warning_center, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label_warning_center, lv_color_white(), 0);
    lv_obj_set_style_text_align(label_warning_center, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label_warning_center,
                      "Bajar joystick\nizquierdo a 0\npara armar");

/* ================= HOME ================= */
    home = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(home, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(home, LV_OPA_100, 0);

    label_home = lv_label_create(home);
    lv_obj_set_align(label_home, LV_ALIGN_CENTER);
    lv_obj_set_height(label_home, LV_SIZE_CONTENT);
    lv_obj_set_width(label_home, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_home, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label_home, lv_color_white(), 0);
    lv_obj_set_y(label_home, 90);
    lv_label_set_text(label_home, "< HOME >");

    label_home_center = lv_label_create(home);
    lv_obj_set_align(label_home_center, LV_ALIGN_CENTER);
    lv_obj_set_height(label_home_center, LV_SIZE_CONTENT);
    lv_obj_set_width(label_home_center, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_home_center, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(label_home_center, lv_color_white(), 0);
    lv_obj_set_style_text_align(label_home_center, LV_FLEX_ALIGN_CENTER, 0);
    lv_label_set_text(label_home_center, "MOTOR\n0%");

    lv_style_init(&style_arc_panel);
    lv_style_set_arc_color(&style_arc_panel, lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_arc_rounded(&style_arc_panel, 1);

    arc_panel = lv_arc_create(home);
    lv_obj_set_size(arc_panel, 240, 240);
    lv_arc_set_rotation(arc_panel, 135);
    lv_arc_set_bg_angles(arc_panel, 0, 270);
    lv_arc_set_value(arc_panel, 0);
    lv_obj_remove_style(arc_panel, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_panel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(arc_panel);
    lv_obj_add_style(arc_panel, &style_arc_panel, LV_PART_INDICATOR);
    lv_event_send(arc_panel, LV_EVENT_VALUE_CHANGED, NULL);

    lv_style_init(&style_arc_motor);
    lv_style_set_arc_color(&style_arc_motor, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_arc_rounded(&style_arc_motor, 1);

    arc_motor = lv_arc_create(home);
    lv_obj_set_size(arc_motor, 200, 200);
    lv_arc_set_rotation(arc_motor, 135);
    lv_arc_set_bg_angles(arc_motor, 0, 270);
    lv_arc_set_value(arc_motor, 0);
    lv_obj_remove_style(arc_motor, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_motor, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(arc_motor);
    lv_obj_add_style(arc_motor, &style_arc_motor, LV_PART_INDICATOR);
    lv_event_send(arc_motor, LV_EVENT_VALUE_CHANGED, NULL);

    lv_anim_init(&anim_arc);
    lv_anim_set_var(&anim_arc, arc_motor);
    lv_anim_set_time(&anim_arc, 200);
    lv_anim_set_exec_cb(&anim_arc, set_angle_arc_motor);

/* ================= HOME CHART ================= */
    home_chart = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(home_chart, lv_color_black(), 0);

    label_home_chart = lv_label_create(home_chart);
    lv_obj_set_align(label_home_chart, LV_ALIGN_CENTER);
    lv_obj_set_height(label_home_chart, LV_SIZE_CONTENT);
    lv_obj_set_width(label_home_chart, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_home_chart, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label_home_chart, lv_color_white(), 0);
    lv_obj_set_y(label_home_chart, 90);
    lv_label_set_text(label_home_chart, "< HOME >");
    lv_obj_set_style_bg_color(label_home_chart, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(label_home_chart, LV_OPA_70, 0);
    lv_obj_move_foreground(label_home_chart);

    chart_home_chart = lv_chart_create(home_chart);
    lv_obj_set_align(chart_home_chart, LV_ALIGN_OUT_LEFT_MID);
    lv_obj_set_size(chart_home_chart, 175, 240);
    lv_chart_set_type(chart_home_chart, LV_CHART_TYPE_LINE);
    lv_obj_set_style_bg_opa(chart_home_chart, LV_OPA_0, 0);
    lv_chart_set_div_line_count(chart_home_chart, 0, 0);
    lv_obj_set_style_border_opa(chart_home_chart, LV_OPA_0, 0);
    lv_chart_set_update_mode(chart_home_chart, LV_CHART_UPDATE_MODE_SHIFT);
    ser1 = lv_chart_add_series(chart_home_chart,
                               lv_palette_main(LV_PALETTE_RED),
                               LV_CHART_AXIS_PRIMARY_Y);
    ser2 = lv_chart_add_series(chart_home_chart,
                               lv_palette_main(LV_PALETTE_GREEN),
                               LV_CHART_AXIS_SECONDARY_Y);
    lv_chart_refresh(chart_home_chart);

    label_chart_home_chart_right = lv_label_create(home_chart);
    lv_obj_align(label_chart_home_chart_right, LV_ALIGN_RIGHT_MID, -5, 0);
    lv_obj_set_height(label_chart_home_chart_right, LV_SIZE_CONTENT);
    lv_obj_set_width(label_chart_home_chart_right, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_chart_home_chart_right, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label_chart_home_chart_right, lv_color_white(), 0);
    lv_label_set_recolor(label_chart_home_chart_right, true);
    lv_label_set_text(label_chart_home_chart_right,
                      "#F44336 PANEL#\n#4CAF50 MOTOR#");
    lv_obj_set_style_text_align(label_chart_home_chart_right, LV_TEXT_ALIGN_RIGHT, 0);

/* ================= PANEL ================= */
    screen_panel = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_panel, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen_panel, LV_OPA_100, 0);

    label_panel = lv_label_create(screen_panel);
    lv_obj_set_align(label_panel, LV_ALIGN_CENTER);
    lv_obj_set_height(label_panel, LV_SIZE_CONTENT);
    lv_obj_set_width(label_panel, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_panel, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label_panel, lv_color_white(), 0);
    lv_obj_set_y(label_panel, 90);
    lv_label_set_text(label_panel, "< PANEL >");

    label_panel_center = lv_label_create(screen_panel);
    lv_obj_set_align(label_panel_center, LV_ALIGN_CENTER);
    lv_obj_set_height(label_panel_center, LV_SIZE_CONTENT);
    lv_obj_set_width(label_panel_center, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_panel_center, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label_panel_center, lv_color_white(), 0);
    lv_obj_set_style_text_align(label_panel_center, LV_FLEX_ALIGN_CENTER, 0);
    lv_label_set_text_fmt(label_panel_center, "%.1f V\n%.1f A\n%.1f W",
                          0.0, 0.0, 0.0);

    meter_panel = lv_meter_create(screen_panel);
    lv_obj_center(meter_panel);
    lv_obj_set_size(meter_panel, 240, 240);
    lv_obj_set_style_bg_opa(meter_panel, LV_OPA_0, 0);
    lv_obj_remove_style(meter_panel, NULL, LV_PART_INDICATOR);
    lv_obj_set_style_border_opa(meter_panel, LV_OPA_0, 0);

    lv_meter_scale_t *scale_meter_panel = lv_meter_add_scale(meter_panel);
    lv_meter_set_scale_ticks(meter_panel, scale_meter_panel, 40, 2, 30,
                             lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_range(meter_panel, scale_meter_panel, 0, 100, 270, 135);

    indic_meter_panel = lv_meter_add_scale_lines(
        meter_panel, scale_meter_panel,
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_RED),
        false, 4);
    lv_meter_set_indicator_start_value(meter_panel, indic_meter_panel, 0);
    lv_meter_set_indicator_end_value(meter_panel, indic_meter_panel, 0);

    lv_anim_init(&anim_panel);
    lv_anim_set_var(&anim_panel, meter_panel);
    lv_anim_set_time(&anim_panel, 1000);
    lv_anim_set_exec_cb(&anim_panel, set_value_panel);

/* ================= MOTOR ================= */
    screen_motor = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_motor, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen_motor, LV_OPA_100, 0);

    label_motor = lv_label_create(screen_motor);
    lv_obj_set_align(label_motor, LV_ALIGN_CENTER);
    lv_obj_set_height(label_motor, LV_SIZE_CONTENT);
    lv_obj_set_width(label_motor, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_motor, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label_motor, lv_color_white(), 0);
    lv_obj_set_y(label_motor, 90);
    lv_label_set_text(label_motor, "< MOTOR >");

    label_motor_center = lv_label_create(screen_motor);
    lv_obj_set_align(label_motor_center, LV_ALIGN_CENTER);
    lv_obj_set_height(label_motor_center, LV_SIZE_CONTENT);
    lv_obj_set_width(label_motor_center, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_motor_center, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label_motor_center, lv_color_white(), 0);
    lv_obj_set_style_text_align(label_motor_center, LV_FLEX_ALIGN_CENTER, 0);
    lv_label_set_text_fmt(label_motor_center, "%.1f V\n%.1f A\n%.1f W",
                          0.0, 0.0, 0.0);

    meter_motor = lv_meter_create(screen_motor);
    lv_obj_center(meter_motor);
    lv_obj_set_size(meter_motor, 240, 240);
    lv_obj_set_style_bg_opa(meter_motor, LV_OPA_0, 0);
    lv_obj_remove_style(meter_motor, NULL, LV_PART_INDICATOR);
    lv_obj_set_style_border_opa(meter_motor, LV_OPA_0, 0);

    lv_meter_scale_t *scale_meter_motor = lv_meter_add_scale(meter_motor);
    lv_meter_set_scale_ticks(meter_motor, scale_meter_motor, 40, 2, 30,
                             lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_range(meter_motor, scale_meter_motor, 0, 100, 270, 135);

    indic_meter_motor = lv_meter_add_scale_lines(
        meter_motor, scale_meter_motor,
        lv_palette_main(LV_PALETTE_AMBER),
        lv_palette_main(LV_PALETTE_RED),
        false, 4);
    lv_meter_set_indicator_start_value(meter_motor, indic_meter_motor, 0);
    lv_meter_set_indicator_end_value(meter_motor, indic_meter_motor, 0);

    lv_anim_init(&anim_motor);
    lv_anim_set_var(&anim_motor, meter_motor);
    lv_anim_set_time(&anim_motor, 1000);
    lv_anim_set_exec_cb(&anim_motor, set_value_motor);

/* ================= BATERIA ================= */
    screen_bateria = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_bateria, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen_bateria, LV_OPA_100, 0);

    label_bateria = lv_label_create(screen_bateria);
    lv_obj_set_align(label_bateria, LV_ALIGN_CENTER);
    lv_obj_set_height(label_bateria, LV_SIZE_CONTENT);
    lv_obj_set_width(label_bateria, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_bateria, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label_bateria, lv_color_white(), 0);
    lv_obj_set_y(label_bateria, 90);
    lv_label_set_text(label_bateria, "< BATERIA >");

    lv_style_init(&style_bar_bateria);
    lv_style_set_bg_opa(&style_bar_bateria, LV_OPA_COVER);
    lv_style_set_bg_color(&style_bar_bateria, lv_color_white());
    lv_style_set_radius(&style_bar_bateria, 0);

    lv_style_init(&style_bar_bateria_border);
    lv_style_set_bg_opa(&style_bar_bateria_border, LV_OPA_0);
    lv_style_set_border_color(&style_bar_bateria_border, lv_color_white());
    lv_style_set_border_width(&style_bar_bateria_border, 1);
    lv_style_set_pad_all(&style_bar_bateria_border, 5);
    lv_style_set_radius(&style_bar_bateria_border, 0);

    bar_bateria = lv_bar_create(screen_bateria);
    lv_obj_set_size(bar_bateria, 100, 30);
    lv_obj_center(bar_bateria);
    lv_bar_set_range(bar_bateria, 36, 42);
    lv_obj_align(bar_bateria, LV_ALIGN_TOP_MID, 0, 50);
    lv_bar_set_value(bar_bateria, 40, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_bateria, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar_bateria, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar_bateria, LV_OPA_100, LV_PART_INDICATOR);
    lv_obj_set_style_border_color(bar_bateria, lv_color_white(), 0);
    lv_obj_set_style_border_width(bar_bateria, 1, 0);
    lv_obj_set_style_pad_all(bar_bateria, 5, 0);
    lv_obj_set_style_radius(bar_bateria, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_bateria, 0, LV_PART_INDICATOR);

    label_bateria_left = lv_label_create(screen_bateria);
    lv_obj_align(label_bateria_left, LV_ALIGN_LEFT_MID, 20, 0);
    lv_obj_set_height(label_bateria_left, LV_SIZE_CONTENT);
    lv_obj_set_width(label_bateria_left, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_bateria_left, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label_bateria_left, lv_color_white(), 0);
    lv_label_set_recolor(label_bateria_left, true);
    lv_label_set_text_fmt(label_bateria_left,
                          "#%s " LV_SYMBOL_BATTERY_FULL "# Mando\n#%s " LV_SYMBOL_BATTERY_2 "# Barco",
                          "33ff66", "3366ff");
    lv_obj_set_style_text_align(label_bateria_left, LV_TEXT_ALIGN_LEFT, 0);

    label_bateria_right = lv_label_create(screen_bateria);
    lv_obj_align(label_bateria_right, LV_ALIGN_RIGHT_MID, -30, 0);
    lv_obj_set_height(label_bateria_right, LV_SIZE_CONTENT);
    lv_obj_set_width(label_bateria_right, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_bateria_right, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label_bateria_right, lv_color_white(), 0);
    lv_label_set_recolor(label_bateria_right, true);
    lv_label_set_text_fmt(label_bateria_right, "%.2f V\n%.2f V", 4.0, 7.6);
    lv_obj_set_style_text_align(label_bateria_right, LV_TEXT_ALIGN_RIGHT, 0);

    lv_anim_init(&anim_bateria);
    lv_anim_set_var(&anim_bateria, bar_bateria);
    lv_anim_set_time(&anim_bateria, 1000);
    lv_anim_set_exec_cb(&anim_bateria, set_value_bateria);

/* ================= GPS ================= */
    screen_gps = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_gps, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen_gps, LV_OPA_100, 0);

    label_gps = lv_label_create(screen_gps);
    lv_obj_set_align(label_gps, LV_ALIGN_CENTER);
    lv_obj_set_height(label_gps, LV_SIZE_CONTENT);
    lv_obj_set_width(label_gps, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_gps, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label_gps, lv_color_white(), 0);
    lv_obj_set_y(label_gps, 90);
    lv_label_set_text(label_gps, "< GPS >");

    label_gps_center = lv_label_create(screen_gps);
    lv_obj_set_align(label_gps_center, LV_ALIGN_CENTER);
    lv_obj_set_height(label_gps_center, LV_SIZE_CONTENT);
    lv_obj_set_width(label_gps_center, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_gps_center, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label_gps_center, lv_color_white(), 0);
    lv_obj_set_style_text_align(label_gps_center, LV_FLEX_ALIGN_CENTER, 0);
    lv_label_set_text_fmt(label_gps_center,
                          "Latitud\n%f\nLongitud\n%f",
                          38.3451700, -0.4814900);

    meter_gps = lv_meter_create(screen_gps);
    lv_obj_set_size(meter_gps, 240, 240);
    lv_obj_center(meter_gps);
    lv_obj_set_style_bg_opa(meter_gps, LV_OPA_0, 0);
    lv_obj_remove_style(meter_gps, NULL, LV_PART_INDICATOR);
    lv_obj_set_style_opa(meter_gps, LV_OPA_70, 0);

    scale_gps = lv_meter_add_scale(meter_gps);
    lv_meter_set_scale_ticks(meter_gps, scale_gps, 61, 1, 10,
                             lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(meter_gps, scale_gps, 15, 4, 20,
                                   lv_palette_main(LV_PALETTE_RED), -100);
    lv_meter_set_scale_range(meter_gps, scale_gps, 0, 60, 360, 255);

    lv_obj_move_foreground(label_gps);

    lv_anim_init(&anim_gps);
    lv_anim_set_var(&anim_gps, scale_gps);
    lv_anim_set_values(&anim_gps, 255, 285);
    lv_anim_set_exec_cb(&anim_gps, set_angle_gps);
    lv_anim_set_time(&anim_gps, 2000);
    lv_anim_set_playback_time(&anim_gps, 2000);
    lv_anim_set_repeat_count(&anim_gps, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&anim_gps);

/* ================= RELOJ ================= */
    screen_reloj = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_reloj, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen_reloj, LV_OPA_100, 0);

    label_reloj = lv_label_create(screen_reloj);
    lv_obj_set_align(label_reloj, LV_ALIGN_CENTER);
    lv_obj_set_height(label_reloj, LV_SIZE_CONTENT);
    lv_obj_set_width(label_reloj, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_reloj, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label_reloj, lv_color_white(), 0);
    lv_obj_set_y(label_reloj, 90);
    lv_label_set_text(label_reloj, "< RELOJ >");

    label_reloj_center = lv_label_create(screen_reloj);
    lv_obj_set_align(label_reloj_center, LV_ALIGN_CENTER);
    lv_obj_set_height(label_reloj_center, LV_SIZE_CONTENT);
    lv_obj_set_width(label_reloj_center, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_reloj_center, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(label_reloj_center, lv_color_white(), 0);
    lv_obj_set_style_text_align(label_reloj_center, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_border_color(label_reloj_center, lv_color_white(), 0);
    lv_obj_set_style_border_width(label_reloj_center, 2, 0);
    lv_obj_set_style_border_opa(label_reloj_center, LV_OPA_50, 0);
    lv_obj_set_style_border_side(label_reloj_center,
                                 LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_pad_all(label_reloj_center, 25, 0);
    lv_label_set_text(label_reloj_center, "16 : 45");

    label_reloj_top = lv_label_create(screen_reloj);
    lv_obj_set_align(label_reloj_top, LV_ALIGN_TOP_MID);
    lv_obj_set_height(label_reloj_top, LV_SIZE_CONTENT);
    lv_obj_set_width(label_reloj_top, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_reloj_top, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label_reloj_top, lv_color_white(), 0);
    lv_obj_set_style_text_align(label_reloj_top, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_all(label_reloj_top, 20, 0);
    lv_label_set_text(label_reloj_top, "3 Diciembre 2024");

/* ================= CALIBRATE ================= */
    screen_calibrate = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_calibrate, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen_calibrate, LV_OPA_100, 0);

    label_calibrate = lv_label_create(screen_calibrate);
    lv_obj_set_align(label_calibrate, LV_ALIGN_CENTER);
    lv_obj_set_height(label_calibrate, LV_SIZE_CONTENT);
    lv_obj_set_width(label_calibrate, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_calibrate, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label_calibrate, lv_color_white(), 0);
    lv_obj_set_y(label_calibrate, 90);
    lv_label_set_text(label_calibrate, "< CALIBRATE >");

    label_calibrate_center = lv_label_create(screen_calibrate);
    lv_obj_set_align(label_calibrate_center, LV_ALIGN_CENTER);
    lv_obj_set_height(label_calibrate_center, LV_SIZE_CONTENT);
    lv_obj_set_width(label_calibrate_center, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_calibrate_center, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label_calibrate_center, lv_color_white(), 0);
    lv_obj_set_style_text_align(label_calibrate_center, LV_FLEX_ALIGN_CENTER, 0);
    lv_label_set_text(label_calibrate_center, "Calibrar joysticks?");

/* ================= MENU ================= */
    screen_menu = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_menu, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen_menu, LV_OPA_100, 0);

    label_menu = lv_label_create(screen_menu);
    lv_obj_set_align(label_menu, LV_ALIGN_CENTER);
    lv_obj_set_height(label_menu, LV_SIZE_CONTENT);
    lv_obj_set_width(label_menu, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_menu, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label_menu, lv_color_white(), 0);
    lv_obj_set_y(label_menu, 90);
    lv_label_set_text(label_menu, "< MENU >");

    label_menu_center = lv_label_create(screen_menu);
    lv_obj_set_align(label_menu_center, LV_ALIGN_CENTER);
    lv_obj_set_height(label_menu_center, LV_SIZE_CONTENT);
    lv_obj_set_width(label_menu_center, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(label_menu_center, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(label_menu_center, lv_color_white(), 0);
    lv_obj_set_style_text_align(label_menu_center, LV_FLEX_ALIGN_CENTER, 0);
    lv_label_set_text(label_menu_center, "CONFIGURACION");

/* ================= MENU CONFIG ================= */
    screen_menu_config = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_menu_config, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen_menu_config, LV_OPA_100, 0);

    list_menu_config = lv_list_create(screen_menu_config);
    lv_obj_set_size(list_menu_config, 240, 240);
    lv_obj_center(list_menu_config);
    lv_obj_set_style_bg_color(list_menu_config, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(list_menu_config, LV_OPA_100, 0);
    lv_obj_set_style_border_width(list_menu_config, 0, 0);

    lv_obj_t *btn;

    lv_list_add_text(list_menu_config, "Joysticks");
    btn = lv_list_add_btn(list_menu_config, LV_SYMBOL_REFRESH, "Calibrar");
    lv_obj_add_event_cb(btn, display_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn, lv_color_white(), 0);

    lv_list_add_text(list_menu_config, "Pantallas");

    btn = lv_list_add_btn(list_menu_config, LV_SYMBOL_HOME, "Home");
    lv_obj_add_event_cb(btn, display_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn, lv_color_white(), 0);
    lv_obj_add_state(btn, LV_STATE_CHECKED);

    btn = lv_list_add_btn(list_menu_config, LV_SYMBOL_HOME, "Home 2");
    lv_obj_add_event_cb(btn, display_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn, lv_color_white(), 0);

    btn = lv_list_add_btn(list_menu_config, LV_SYMBOL_CHARGE, "Panel");
    lv_obj_add_event_cb(btn, display_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn, lv_color_white(), 0);

    btn = lv_list_add_btn(list_menu_config, LV_SYMBOL_SETTINGS, "Motor");
    lv_obj_add_event_cb(btn, display_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn, lv_color_white(), 0);

    btn = lv_list_add_btn(list_menu_config, LV_SYMBOL_BATTERY_FULL, "Bateria");
    lv_obj_add_event_cb(btn, display_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn, lv_color_white(), 0);

    btn = lv_list_add_btn(list_menu_config, LV_SYMBOL_GPS, "GPS");
    lv_obj_add_event_cb(btn, display_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn, lv_color_white(), 0);

    btn = lv_list_add_btn(list_menu_config, LV_SYMBOL_BELL, "Reloj");
    lv_obj_add_event_cb(btn, display_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn, lv_color_white(), 0);

    lv_list_add_text(list_menu_config, "Exit");

    btn = lv_list_add_btn(list_menu_config, LV_SYMBOL_OK, "Apply");
    lv_obj_add_event_cb(btn, display_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn, lv_color_white(), 0);
}

#endif /* DISPLAYS_H */