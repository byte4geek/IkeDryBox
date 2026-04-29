// File: src/ui_settings.cpp
#include "globals.h"
#include "ui_settings.h"
#include <WiFi.h>

lv_obj_t *tabview, *kb;
lv_obj_t *slider_kp, *slider_ki, *slider_kd;
lv_obj_t *lbl_val_kp, *lbl_val_ki, *lbl_val_kd;

lv_obj_t *ta_ip, *ta_gw, *ta_nm; 
lv_obj_t *ta_mqtt_host, *ta_mqtt_port, *ta_mqtt_user, *ta_mqtt_pass;

lv_obj_t *btn_save, *btn_cancel;

// New objects for the CUSTOM TAB
lv_obj_t *slider_c_temp, *slider_c_time;
lv_obj_t *lbl_val_c_temp, *lbl_val_c_time;

void open_settings_cb(lv_event_t *e) {
    lv_slider_set_value(slider_kp, (int)Kp, LV_ANIM_OFF);
    lv_slider_set_value(slider_ki, (int)(Ki * 100), LV_ANIM_OFF);
    lv_slider_set_value(slider_kd, (int)(Kd * 10), LV_ANIM_OFF);

    lv_label_set_text_fmt(lbl_val_kp, "Kp: %d", (int)Kp);
    lv_label_set_text_fmt(lbl_val_ki, "Ki: %s", String(Ki, 2).c_str());
    lv_label_set_text_fmt(lbl_val_kd, "Kd: %s", String(Kd, 1).c_str());

    // Initial Custom slider settings
    lv_slider_set_value(slider_c_temp, (int)custom_temp, LV_ANIM_OFF);
    lv_label_set_text_fmt(lbl_val_c_temp, "Temp: %d C", (int)custom_temp);
    
    lv_slider_set_value(slider_c_time, (int)(custom_time / 3600), LV_ANIM_OFF);
    lv_label_set_text_fmt(lbl_val_c_time, "Time: %d h", (int)(custom_time / 3600));

    // DHCP SYNCHRONIZATION ON THE DISPLAY
    if(use_dhcp) lv_obj_add_state(cb_dhcp, LV_STATE_CHECKED);
    else lv_obj_clear_state(cb_dhcp, LV_STATE_CHECKED);

    prefs.begin("drybox", false);

    // Load the current name into the text field
    lv_textarea_set_text(ta_hostname, hostname.c_str());
    
    // If we use DHCP we show the true IP and block the fields, otherwise we load the saved ones
    if(use_dhcp) {
        lv_textarea_set_text(ta_ip, WiFi.localIP().toString().c_str());
        lv_textarea_set_text(ta_gw, WiFi.gatewayIP().toString().c_str());
        lv_textarea_set_text(ta_nm, WiFi.subnetMask().toString().c_str());
        lv_obj_add_state(ta_ip, LV_STATE_DISABLED);
        lv_obj_add_state(ta_gw, LV_STATE_DISABLED);
        lv_obj_add_state(ta_nm, LV_STATE_DISABLED);
    } else {
        lv_textarea_set_text(ta_ip, prefs.getString("ip", "192.168.1.100").c_str());
        lv_textarea_set_text(ta_gw, prefs.getString("gw", "192.168.1.1").c_str());
        lv_textarea_set_text(ta_nm, prefs.getString("nm", "255.255.255.0").c_str());
        lv_obj_clear_state(ta_ip, LV_STATE_DISABLED);
        lv_obj_clear_state(ta_gw, LV_STATE_DISABLED);
        lv_obj_clear_state(ta_nm, LV_STATE_DISABLED);
    }

    // MQTT load
    lv_textarea_set_text(ta_mqtt_host, prefs.getString("mqtt", "broker.hivemq.com").c_str());
    lv_textarea_set_text(ta_mqtt_port, prefs.getString("m_port", "1883").c_str());
    lv_textarea_set_text(ta_mqtt_user, prefs.getString("m_user", "").c_str());
    lv_textarea_set_text(ta_mqtt_pass, prefs.getString("m_pass", "").c_str());
    prefs.end();

    lv_scr_load(settings_screen);
}

void dhcp_event_cb(lv_event_t * e) {
    lv_obj_t * cb = lv_event_get_target(e);
    use_dhcp = lv_obj_has_state(cb, LV_STATE_CHECKED);
    
    if(use_dhcp) {
        lv_textarea_set_text(ta_ip, WiFi.localIP().toString().c_str());
        lv_textarea_set_text(ta_gw, WiFi.gatewayIP().toString().c_str());
        lv_textarea_set_text(ta_nm, WiFi.subnetMask().toString().c_str());
        lv_obj_add_state(ta_ip, LV_STATE_DISABLED);
        lv_obj_add_state(ta_gw, LV_STATE_DISABLED);
        lv_obj_add_state(ta_nm, LV_STATE_DISABLED);
    } else {
        lv_obj_clear_state(ta_ip, LV_STATE_DISABLED);
        lv_obj_clear_state(ta_gw, LV_STATE_DISABLED);
        lv_obj_clear_state(ta_nm, LV_STATE_DISABLED);
    }
}

void slider_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    if (slider == slider_kp) {
        Kp = lv_slider_get_value(slider_kp);
        lv_label_set_text_fmt(lbl_val_kp, "Kp: %d", (int)Kp);
    } else if (slider == slider_ki) {
        Ki = lv_slider_get_value(slider_ki) / 100.0;
        lv_label_set_text_fmt(lbl_val_ki, "Ki: %s", String(Ki, 2).c_str());
    } else if (slider == slider_kd) {
        Kd = lv_slider_get_value(slider_kd) / 10.0;
        lv_label_set_text_fmt(lbl_val_kd, "Kd: %s", String(Kd, 1).c_str());
    } else if (slider == slider_c_temp) {
        int val = lv_slider_get_value(slider_c_temp);
        lv_label_set_text_fmt(lbl_val_c_temp, "Temp: %d C", val);
    } else if (slider == slider_c_time) {
        int val = lv_slider_get_value(slider_c_time);
        lv_label_set_text_fmt(lbl_val_c_time, "Time: %d h", val);
    }
}

void save_settings_cb(lv_event_t *e) {
    prefs.begin("drybox", false);
    
    // 1. Sync Hostname
    hostname = lv_textarea_get_text(ta_hostname);
    prefs.putString("hostname", hostname);

    // 2. Save PID
    prefs.putDouble("kp", Kp);
    prefs.putDouble("ki", Ki);
    prefs.putDouble("kd", Kd);
    
    // 3. Save Custom Profile
    custom_temp = (float)lv_slider_get_value(slider_c_temp);
    custom_time = (long)lv_slider_get_value(slider_c_time) * 3600;
    prefs.putFloat("c_temp", custom_temp);
    prefs.putLong("c_time", custom_time);

    // 4. Save Network
    prefs.putBool("dhcp", use_dhcp); 
    if(!use_dhcp) {
        prefs.putString("ip", lv_textarea_get_text(ta_ip));
        prefs.putString("gw", lv_textarea_get_text(ta_gw));
        prefs.putString("nm", lv_textarea_get_text(ta_nm));
    }
    
    // 5. Save MQTT
    prefs.putString("mqtt", lv_textarea_get_text(ta_mqtt_host));
    prefs.putString("m_port", lv_textarea_get_text(ta_mqtt_port));
    prefs.putString("m_user", lv_textarea_get_text(ta_mqtt_user));
    prefs.putString("m_pass", lv_textarea_get_text(ta_mqtt_pass));
    
    prefs.end();

    // Apply PID now (for security)
    boxPID.SetTunings(Kp, Ki, Kd);

    // FUNDAMENTAL RESTART
    // Show a quick message or change the color of the feedback button
    lv_label_set_text(lv_obj_get_child(btn_save, 0), "REBOOTING...");
    lv_obj_set_style_bg_color(btn_save, lv_color_hex(0xFF0000), 0);
    
    // Short pause to show the message and then reset
    delay(1000);
    ESP.restart();
}

void cancel_settings_cb(lv_event_t *e) {
    prefs.begin("drybox", false);
    Kp = prefs.getDouble("kp", 113.0);
    Ki = prefs.getDouble("ki", 0.80);
    Kd = prefs.getDouble("kd", 3.2);
    use_dhcp = prefs.getBool("dhcp", true);
    prefs.end();
    lv_scr_load(main_screen);
}

static void ta_event_cb(lv_event_t *e) {
    lv_obj_t *ta = lv_event_get_target(e);
    if (e->code == LV_EVENT_FOCUSED) {
        if(lv_obj_has_state(ta, LV_STATE_DISABLED)) return;

        lv_keyboard_set_textarea(kb, ta);
        if(ta == ta_ip || ta == ta_gw || ta == ta_nm || ta == ta_mqtt_port) {
            lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
        } else {
            lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
        }
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN); 
        lv_obj_add_flag(btn_save, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(btn_cancel, LV_OBJ_FLAG_HIDDEN);
        
        lv_obj_set_height(tabview, 120); 
        lv_obj_scroll_to_view(ta, LV_ANIM_ON);
    }
    if (e->code == LV_EVENT_READY || e->code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN); 
        lv_obj_clear_state(ta, LV_STATE_FOCUSED);
        lv_obj_clear_flag(btn_save, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(btn_cancel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_height(tabview, 240);
    }
}

void build_settings_ui() {
    settings_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(settings_screen, lv_color_hex(0x000000), 0);

    tabview = lv_tabview_create(settings_screen, LV_DIR_TOP, 30);
    lv_obj_set_style_bg_color(tabview, lv_color_hex(0x222222), LV_PART_MAIN);

    lv_obj_t *t1 = lv_tabview_add_tab(tabview, "PID");
    lv_obj_t *t4 = lv_tabview_add_tab(tabview, "Custom"); // Placed here next to the PID
    lv_obj_t *t2 = lv_tabview_add_tab(tabview, "Net"); // Shortened to fit 4 tabs
    lv_obj_t *t3 = lv_tabview_add_tab(tabview, "MQTT");

    // --- TAB 1: PID ---
    lbl_val_kp = lv_label_create(t1); lv_obj_set_style_text_color(lbl_val_kp, lv_color_hex(0xFFFFFF), 0); lv_obj_align(lbl_val_kp, LV_ALIGN_TOP_LEFT, 0, 10);
    slider_kp = lv_slider_create(t1); lv_slider_set_range(slider_kp, 0, 150); lv_obj_set_size(slider_kp, 180, 15); lv_obj_align(slider_kp, LV_ALIGN_TOP_LEFT, 90, 10); lv_obj_add_event_cb(slider_kp, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lbl_val_ki = lv_label_create(t1); lv_obj_set_style_text_color(lbl_val_ki, lv_color_hex(0xFFFFFF), 0); lv_obj_align(lbl_val_ki, LV_ALIGN_TOP_LEFT, 0, 50);
    slider_ki = lv_slider_create(t1); lv_slider_set_range(slider_ki, 0, 200); lv_obj_set_size(slider_ki, 180, 15); lv_obj_align(slider_ki, LV_ALIGN_TOP_LEFT, 90, 50); lv_obj_add_event_cb(slider_ki, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lbl_val_kd = lv_label_create(t1); lv_obj_set_style_text_color(lbl_val_kd, lv_color_hex(0xFFFFFF), 0); lv_obj_align(lbl_val_kd, LV_ALIGN_TOP_LEFT, 0, 90);
    slider_kd = lv_slider_create(t1); lv_slider_set_range(slider_kd, 0, 200); lv_obj_set_size(slider_kd, 180, 15); lv_obj_align(slider_kd, LV_ALIGN_TOP_LEFT, 90, 90); lv_obj_add_event_cb(slider_kd, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // --- TAB 4: CUSTOM PROFILE ---
    lbl_val_c_temp = lv_label_create(t4);
    lv_obj_set_style_text_color(lbl_val_c_temp, lv_color_hex(0x00FF00), 0);
    lv_obj_align(lbl_val_c_temp, LV_ALIGN_TOP_LEFT, 0, 10);
    lv_label_set_text_fmt(lbl_val_c_temp, "Temp: 50 C");

    slider_c_temp = lv_slider_create(t4);
    lv_slider_set_range(slider_c_temp, 20, 90);
    lv_obj_set_size(slider_c_temp, 160, 15); // Slightly narrower
    lv_obj_align(slider_c_temp, LV_ALIGN_TOP_LEFT, 110, 10);
    lv_obj_add_event_cb(slider_c_temp, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lbl_val_c_time = lv_label_create(t4);
    lv_obj_set_style_text_color(lbl_val_c_time, lv_color_hex(0x00FF00), 0);
    lv_obj_align(lbl_val_c_time, LV_ALIGN_TOP_LEFT, 0, 60);
    lv_label_set_text_fmt(lbl_val_c_time, "Time: 5 h");

    slider_c_time = lv_slider_create(t4);
    lv_slider_set_range(slider_c_time, 1, 24);
    lv_obj_set_size(slider_c_time, 160, 15);
    lv_obj_align(slider_c_time, LV_ALIGN_TOP_LEFT, 110, 60);
    lv_obj_add_event_cb(slider_c_time, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // --- TAB 2: NETWORK ---
    cb_dhcp = lv_checkbox_create(t2);
    lv_checkbox_set_text(cb_dhcp, "Use DHCP");
    lv_obj_set_style_text_color(cb_dhcp, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(cb_dhcp, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_event_cb(cb_dhcp, dhcp_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    auto create_ta = [&](lv_obj_t* parent, const char* label, int y) -> lv_obj_t* {
        lv_obj_t* l = lv_label_create(parent); lv_label_set_text(l, label);
        lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), 0); lv_obj_align(l, LV_ALIGN_TOP_LEFT, 0, y + 10);
        lv_obj_t* ta = lv_textarea_create(parent); lv_textarea_set_one_line(ta, true);
        lv_obj_set_width(ta, 160); lv_obj_align(ta, LV_ALIGN_TOP_LEFT, 60, y); // Moved slightly to the right
        lv_obj_add_event_cb(ta, ta_event_cb, LV_EVENT_ALL, NULL);
        return ta;
    };

    prefs.begin("drybox", true); 
    ta_hostname = create_ta(t2, "Name:", 40);
    lv_textarea_set_text(ta_hostname, hostname.c_str());
    ta_ip = create_ta(t2, "IP:", 85);
    ta_gw = create_ta(t2, "GW:", 130);
    ta_nm = create_ta(t2, "NM:", 175);

    // --- TAB 3: MQTT ---
    ta_mqtt_host = create_ta(t3, "Host:", 0);
    ta_mqtt_port = create_ta(t3, "Port:", 45);
    ta_mqtt_user = create_ta(t3, "User:", 90);
    ta_mqtt_pass = create_ta(t3, "Pass:", 135);
    lv_textarea_set_password_mode(ta_mqtt_pass, true);
    prefs.end(); 

    kb = lv_keyboard_create(settings_screen); lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    
    btn_save = lv_btn_create(settings_screen); lv_obj_set_size(btn_save, 100, 35); 
    lv_obj_align(btn_save, LV_ALIGN_BOTTOM_RIGHT, -10, -5); lv_obj_set_style_bg_color(btn_save, lv_color_hex(0x0088FF), 0); 
    lv_obj_add_event_cb(btn_save, save_settings_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_save = lv_label_create(btn_save); lv_label_set_text(lbl_save, "SAVE"); lv_obj_center(lbl_save);

    btn_cancel = lv_btn_create(settings_screen); lv_obj_set_size(btn_cancel, 100, 35); 
    lv_obj_align(btn_cancel, LV_ALIGN_BOTTOM_RIGHT, -120, -5); lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x555555), 0); 
    lv_obj_add_event_cb(btn_cancel, cancel_settings_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_cancel = lv_label_create(btn_cancel); lv_label_set_text(lbl_cancel, "CANCEL"); lv_obj_center(lbl_cancel);
}