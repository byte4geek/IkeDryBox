// File: src/ui_main.cpp
#include "globals.h"
#include "ui_main.h"
#include "ui_settings.h"

// Internal variables only for this screen
lv_obj_t *btn_tar_up, *btn_tar_dn, *btn_tim_up, *btn_tim_dn, *touch_shield;

void update_timer_label() {
    int h = remaining_seconds / 3600;
    int m = (remaining_seconds % 3600) / 60;
    int s = remaining_seconds % 60;
    char buf[16];
    sprintf(buf, "%d:%02d:%02d", h, m, s);
    lv_label_set_text(label_timer, buf);
}

void temp_adj_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    if(btn == btn_tar_up) pid_Setpoint += 1.0;
    else if(btn == btn_tar_dn) pid_Setpoint -= 1.0;
    
    if(pid_Setpoint < 20.0) pid_Setpoint = 20.0;
    if(pid_Setpoint > 90.0) pid_Setpoint = 90.0;
    
    lv_label_set_text_fmt(label_target, "Target: %d C", (int)pid_Setpoint);
}

void time_adj_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    if(btn == btn_tim_up) {
        remaining_seconds += 3600;
    } else if(btn == btn_tim_dn) {
        if(remaining_seconds >= 3600) remaining_seconds -= 3600;
        else remaining_seconds = 0;
    }
    update_timer_label();
}

void dd_event_cb(lv_event_t *e) {
    lv_obj_t *dropdown = lv_event_get_target(e);
    char buf[32];
    lv_dropdown_get_selected_str(dropdown, buf, sizeof(buf));
    
    current_filament = buf; // Sync for MQTT and WebUI

    if (strcmp(buf, "PLA") == 0) { remaining_seconds = 5 * 3600; pid_Setpoint = 50.0; }
    else if (strcmp(buf, "PETG") == 0) { remaining_seconds = 6 * 3600; pid_Setpoint = 65.0; }
    else if (strcmp(buf, "ABS") == 0) { remaining_seconds = 6 * 3600; pid_Setpoint = 80.0; }
    else if (strcmp(buf, "TPU") == 0) { remaining_seconds = 8 * 3600; pid_Setpoint = 55.0; }
    // else if (strcmp(buf, "Custom") == 0) { remaining_seconds = custom_time; pid_Setpoint = custom_temp; }
    else if (strcmp(buf, "Custom") == 0) { remaining_seconds = custom_time; pid_Setpoint = custom_temp; } // <-- 10 SECONDs for debug

    lv_label_set_text_fmt(label_target, "Target: %d C", (int)pid_Setpoint);
    if (!is_running) update_timer_label();
}

void btn_start_event_cb(lv_event_t *e) {
    is_running = !is_running;
    if (is_running) {
        // AUTO-RESET TIMER AT ZERO
        if (remaining_seconds <= 0) {
            current_filament.trim(); 
            
            if (current_filament == "PLA") remaining_seconds = 5 * 3600;
            else if (current_filament == "PETG") remaining_seconds = 6 * 3600;
            else if (current_filament == "ABS") remaining_seconds = 6 * 3600;
            else if (current_filament == "TPU") remaining_seconds = 8 * 3600;
            else if (current_filament == "Custom") remaining_seconds = custom_time;
            else remaining_seconds = 5 * 3600; // Emergency Fallback
            
            // Second safety net: if by any chance it is still <= 0, we force 5 hours
            if (remaining_seconds <= 0) remaining_seconds = 5 * 3600; 
            
            update_timer_label();
        }

        lv_label_set_text(label_btn_start, "STOP");
        lv_obj_set_style_bg_color(btn_start, lv_color_hex(0xCC0000), 0);
        lv_label_set_text(label_status, "Status: DRYING...");
        lv_obj_set_style_text_color(label_status, lv_color_hex(0xFFAA00), 0);
        ledcWrite(PWM_CH_FAN, 255); 
    } else {
        lv_label_set_text(label_btn_start, "START");
        lv_obj_set_style_bg_color(btn_start, lv_color_hex(0x00AA00), 0);
        
        if (remaining_seconds <= 0) {
            lv_label_set_text(label_status, "Status: DONE!");
            lv_obj_set_style_text_color(label_status, lv_color_hex(0x00FF00), 0);
        } else {
            lv_label_set_text(label_status, "Status: READY");
            lv_obj_set_style_text_color(label_status, lv_color_hex(0xAAAAAA), 0);
        }
        
        ledcWrite(PWM_CH_HEATER, 0);
        ledcWrite(PWM_CH_FAN, 0); 
    }
}

// --- Creation of the objects for the main UI
void build_main_ui() {
    main_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x000000), 0);

    lv_obj_t *btn_gear = lv_btn_create(main_screen);
    lv_obj_set_size(btn_gear, 40, 40);
    lv_obj_align(btn_gear, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(btn_gear, lv_color_hex(0x333333), 0);
    lv_obj_add_event_cb(btn_gear, open_settings_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_gear = lv_label_create(btn_gear);
    lv_label_set_text(lbl_gear, LV_SYMBOL_SETTINGS);
    lv_obj_center(lbl_gear);

    lv_obj_t *title = lv_label_create(main_screen);
    lv_label_set_text_fmt(title, "IkeDryBox v%s", FIRMWARE_VERSION); // Centralized firmware version fron globals.h
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFAA00), 0);

    label_temp = lv_label_create(main_screen);
    lv_obj_set_style_text_font(label_temp, &lv_font_montserrat_28, 0);
    lv_obj_align(label_temp, LV_ALIGN_TOP_LEFT, 20, 35);
    lv_label_set_text(label_temp, "--.- C");

    label_hum = lv_label_create(main_screen);
    lv_obj_set_style_text_font(label_hum, &lv_font_montserrat_28, 0);
    lv_obj_align(label_hum, LV_ALIGN_TOP_RIGHT, -60, 35);
    lv_obj_set_style_text_color(label_hum, lv_color_hex(0x00BFFF), 0);
    lv_label_set_text(label_hum, "-- %");

    lv_obj_t *line = lv_line_create(main_screen);
    static lv_point_t pts[] = {{10, 70}, {310, 70}};
    lv_line_set_points(line, pts, 2);
    lv_obj_set_style_line_color(line, lv_color_hex(0x333333), 0);

    lv_obj_t *dd = lv_dropdown_create(main_screen);
    lv_dropdown_set_options(dd, "PLA\nPETG\nABS\nTPU\nCustom"); // Added Custom
    lv_obj_set_width(dd, 110);
    lv_obj_align(dd, LV_ALIGN_TOP_LEFT, 20, 90);
    lv_obj_add_event_cb(dd, dd_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    label_target = lv_label_create(main_screen);
    lv_label_set_text_fmt(label_target, "Target: %d C", (int)pid_Setpoint);
    lv_obj_set_style_text_color(label_target, lv_color_hex(0xFFFFFF), 0); 
    lv_obj_align(label_target, LV_ALIGN_TOP_RIGHT, -20, 80);

    btn_tar_up = lv_btn_create(main_screen); lv_obj_set_size(btn_tar_up, 25, 25);
    lv_obj_align_to(btn_tar_up, label_target, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    lv_obj_set_style_bg_color(btn_tar_up, lv_color_hex(0xFFFFFF), 0); lv_obj_set_style_text_color(btn_tar_up, lv_color_hex(0x000000), 0);
    lv_obj_add_event_cb(btn_tar_up, temp_adj_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_t_up = lv_label_create(btn_tar_up); lv_label_set_text(lbl_t_up, LV_SYMBOL_UP); lv_obj_center(lbl_t_up);

    btn_tar_dn = lv_btn_create(main_screen); lv_obj_set_size(btn_tar_dn, 25, 25);
    lv_obj_align_to(btn_tar_dn, btn_tar_up, LV_ALIGN_OUT_LEFT_MID, -5, 0);
    lv_obj_set_style_bg_color(btn_tar_dn, lv_color_hex(0xFFFFFF), 0); lv_obj_set_style_text_color(btn_tar_dn, lv_color_hex(0x000000), 0);
    lv_obj_add_event_cb(btn_tar_dn, temp_adj_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_t_dn = lv_label_create(btn_tar_dn); lv_label_set_text(lbl_t_dn, LV_SYMBOL_DOWN); lv_obj_center(lbl_t_dn);

    label_timer = lv_label_create(main_screen);
    lv_obj_set_style_text_font(label_timer, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label_timer, lv_color_hex(0xFFFFFF), 0); 
    lv_obj_align(label_timer, LV_ALIGN_TOP_RIGHT, -20, 115);
    update_timer_label();

    btn_tim_up = lv_btn_create(main_screen); lv_obj_set_size(btn_tim_up, 25, 25);
    lv_obj_align_to(btn_tim_up, label_timer, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    lv_obj_set_style_bg_color(btn_tim_up, lv_color_hex(0xFFFFFF), 0); lv_obj_set_style_text_color(btn_tim_up, lv_color_hex(0x000000), 0);
    lv_obj_add_event_cb(btn_tim_up, time_adj_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_tm_up = lv_label_create(btn_tim_up); lv_label_set_text(lbl_tm_up, LV_SYMBOL_UP); lv_obj_center(lbl_tm_up);

    btn_tim_dn = lv_btn_create(main_screen); lv_obj_set_size(btn_tim_dn, 25, 25);
    lv_obj_align_to(btn_tim_dn, btn_tim_up, LV_ALIGN_OUT_LEFT_MID, -5, 0);
    lv_obj_set_style_bg_color(btn_tim_dn, lv_color_hex(0xFFFFFF), 0); lv_obj_set_style_text_color(btn_tim_dn, lv_color_hex(0x000000), 0);
    lv_obj_add_event_cb(btn_tim_dn, time_adj_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_tm_dn = lv_label_create(btn_tim_dn); lv_label_set_text(lbl_tm_dn, LV_SYMBOL_DOWN); lv_obj_center(lbl_tm_dn);

    lv_obj_t *icon_heater = lv_label_create(main_screen);
    lv_label_set_text(icon_heater, LV_SYMBOL_CHARGE " Heater:");
    lv_obj_align(icon_heater, LV_ALIGN_TOP_LEFT, 20, 145);
    lv_obj_set_style_text_color(icon_heater, lv_color_hex(0xFF7700), 0);

    label_heater_per = lv_label_create(main_screen);
    lv_label_set_text(label_heater_per, "0%");
    lv_obj_set_style_text_color(label_heater_per, lv_color_hex(0xFFFFFF), 0); 
    lv_obj_align_to(label_heater_per, icon_heater, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    lv_obj_t *icon_fan = lv_label_create(main_screen);
    lv_label_set_text(icon_fan, LV_SYMBOL_REFRESH " Fan:");
    lv_obj_align(icon_fan, LV_ALIGN_TOP_LEFT, 170, 145);
    lv_obj_set_style_text_color(icon_fan, lv_color_hex(0x00BFFF), 0);

    label_fan_per = lv_label_create(main_screen);
    lv_label_set_text(label_fan_per, "0%");
    lv_obj_set_style_text_color(label_fan_per, lv_color_hex(0xFFFFFF), 0); 
    lv_obj_align_to(label_fan_per, icon_fan, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    label_status = lv_label_create(main_screen);
    lv_label_set_text(label_status, "Status: READY");
    lv_obj_align(label_status, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_set_style_text_color(label_status, lv_color_hex(0xAAAAAA), 0);

    btn_start = lv_btn_create(main_screen);
    lv_obj_set_size(btn_start, 200, 45);
    lv_obj_align(btn_start, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(btn_start, lv_color_hex(0x00AA00), 0);
    lv_obj_add_event_cb(btn_start, btn_start_event_cb, LV_EVENT_CLICKED, NULL);

    label_btn_start = lv_label_create(btn_start);
    lv_label_set_text(label_btn_start, "START SESSION");
    lv_obj_center(label_btn_start);
}