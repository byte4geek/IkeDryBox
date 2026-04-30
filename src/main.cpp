// File: src/main.cpp
#include "globals.h"
#include "ui_main.h"
#include "ui_settings.h"
#include "web_server.h"
#include "mqtt_manager.h"
#include <WiFiManager.h>
#include <Adafruit_SHT31.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// --- EFFECTIVE INSTANTIATION OF GLOBAL VARIABLES ---
Preferences prefs;

double pid_Setpoint = 50.0, pid_Input, pid_Output;
double Kp = 60.0, Ki = 0.90, Kd = 8.0; // The best numbers to start the tuning
PID boxPID(&pid_Input, &pid_Output, &pid_Setpoint, Kp, Ki, Kd, DIRECT);

String hostname = "IkeDryBox"; // default name
lv_obj_t *ta_hostname;

bool is_running = false;
long remaining_seconds = 5 * 3600;

bool use_dhcp = true; 
lv_obj_t *cb_dhcp;

lv_obj_t *main_screen;
lv_obj_t *settings_screen;
lv_obj_t *label_heater_per, *label_fan_per;
lv_obj_t *label_temp, *label_hum, *label_target, *label_status, *label_timer, *btn_start, *label_btn_start;

// --- Backlight
bool auto_screen_off = false;
bool screen_is_off = false;

// --- Custom filament ---
float custom_temp = 50.0; // default 50 °C
long custom_time = 18000; // default 5 hours (in seconds)

// --- CONFIGURATION HARDWARE DISPLAY ---
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9341 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Light_PWM _light_instance;
    lgfx::Touch_XPT2046 _touch_instance;
public:
    LGFX(void) {
        { auto cfg = _bus_instance.config(); cfg.spi_host = SPI2_HOST; cfg.spi_mode = 0; cfg.freq_write = 40000000; cfg.pin_sclk = 14; cfg.pin_mosi = 13; cfg.pin_miso = 12; cfg.pin_dc = 2; cfg.dma_channel = 0; _bus_instance.config(cfg); _panel_instance.setBus(&_bus_instance); }
        { auto cfg = _panel_instance.config(); cfg.pin_cs = 15; cfg.pin_rst = -1; cfg.pin_busy = -1; cfg.memory_width = 320; cfg.memory_height = 240; cfg.panel_width = 320; cfg.panel_height = 240; cfg.offset_rotation = 4; cfg.rgb_order = true; _panel_instance.config(cfg); }
        { auto cfg = _light_instance.config(); cfg.pin_bl = 27; cfg.freq = 44100; cfg.pwm_channel = 7; _light_instance.config(cfg); _panel_instance.setLight(&_light_instance); }
        { auto cfg = _touch_instance.config(); cfg.x_min = 222; cfg.x_max = 3367; cfg.y_min = 192; cfg.y_max = 3732; cfg.bus_shared = true; cfg.spi_host = SPI2_HOST; cfg.pin_cs = 33; _touch_instance.config(cfg); _panel_instance.setTouch(&_touch_instance); }
        setPanel(&_panel_instance);
    }
};

LGFX tft;
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// --- LVGL translator ---
static const uint32_t screenWidth = 320;
static const uint32_t screenHeight = 240;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 15];

void shield_event_cb(lv_event_t * e) {
    // When you tap on the black screen, it does this:
    screen_is_off = false;
    // digitalWrite(BACKLIGHT_PIN, HIGH); // Turn on the light (not working)
    lv_obj_add_flag(touch_shield, LV_OBJ_FLAG_HIDDEN); // Hides the black shield
    
    lv_disp_trig_activity(NULL); // <-- THE CORRECT COMMAND TO RESET THE TIMER
    
    Serial.println(">>> SHIELD TOUCHED: SCREEN WOKEN UP <<<");
}

void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1); uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite(); tft.setAddrWindow(area->x1, area->y1, w, h); tft.pushPixels((uint16_t *)&color_p->full, w * h, true); tft.endWrite();
    lv_disp_flush_ready(disp_drv);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    uint16_t touchX, touchY;
    if (tft.getTouchRaw(&touchX, &touchY)) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = constrain(map(touchX, 300, 3800, 0, 320), 0, 320);
        data->point.y = constrain(map(touchY, 300, 3800, 0, 240), 0, 240);
    } else { data->state = LV_INDEV_STATE_REL; }
}

void load_settings() {
    prefs.begin("drybox", false);
    hostname = prefs.getString("hostname", "IkeDryBox");
    Kp = prefs.getDouble("kp", 113.0);
    Ki = prefs.getDouble("ki", 0.80);
    Kd = prefs.getDouble("kd", 3.2);
    boxPID.SetTunings(Kp, Ki, Kd);
    use_dhcp = prefs.getBool("dhcp", true);
    custom_temp = prefs.getFloat("c_temp", 50.0);
    custom_time = prefs.getLong("c_time", 18000);
    auto_screen_off = prefs.getBool("scr_off", false);

    // If the IP is static, we configure WiFi
    if (!use_dhcp) {
        IPAddress ip, gw, nm;
        ip.fromString(prefs.getString("ip", "192.168.1.100"));
        gw.fromString(prefs.getString("gw", "192.168.1.1"));
        nm.fromString(prefs.getString("nm", "255.255.255.0"));
        WiFi.config(ip, gw, nm);
        WiFi.setHostname(hostname.c_str());
    }
    prefs.end();
}

void setup() {
    Serial.begin(115200);
    
    // pinMode(21, OUTPUT); 
    // digitalWrite(21, LOW);
    pinMode(17, OUTPUT); digitalWrite(17, HIGH);
    pinMode(4, OUTPUT);  digitalWrite(4, HIGH);
    
    pinMode(BACKLIGHT_PIN, OUTPUT);
    digitalWrite(BACKLIGHT_PIN, LOW); // Turn on screen

    load_settings();
    
    // Channel PWM 3 and 4 for the LEDs
    ledcSetup(3, 5000, 8); // Channel for Red
    ledcAttachPin(4, 3);
    ledcSetup(4, 5000, 8); // Channel for Green
    ledcAttachPin(17, 4);

    // We turn everything off at the beginning (HIGH = Off)
    ledcWrite(3, 255); 
    ledcWrite(4, 255);

    ledcSetup(PWM_CH_HEATER, HEATER_FREQ, PWM_RES);
    ledcAttachPin(PIN_HEATER, PWM_CH_HEATER);
    ledcWrite(PWM_CH_HEATER, 0);
    ledcSetup(PWM_CH_FAN, FAN_FREQ, PWM_RES);
    ledcAttachPin(PIN_FAN, PWM_CH_FAN);
    ledcWrite(PWM_CH_FAN, 0);
    boxPID.SetMode(AUTOMATIC); boxPID.SetOutputLimits(0, 255);
    boxPID.SetSampleTime(1000); // Synchronize the PID with your sensor

    tft.init(); tft.setRotation(0); tft.setBrightness(180); tft.fillScreen(0);
    tft.setCursor(20, 20); tft.setTextColor(0xFFAA00); tft.println("Booting IkeDryBox...");

    WiFiManager wm;
    wm.setHostname(hostname.c_str()); // send hostname to the router
    wm.autoConnect(String(hostname + "_Setup").c_str());
    wm.setConfigPortalTimeout(180); // If it doesn't connect within 3 minutes, it exits and restarts
    
    if (!wm.autoConnect("IkeDryBox_Setup")) {
        Serial.println("WiFi connection failed and timeout reached. Force restart...");
        delay(3000);
        ESP.restart();
    }
    Wire.begin(21, 22); sht31.begin(0x44);
    if (!sht31.begin(0x44)) {
        Serial.println("SHT31 sensor error!");
    }

    lv_init(); lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 15);
    static lv_disp_drv_t disp_drv; lv_disp_drv_init(&disp_drv); disp_drv.hor_res = screenWidth; disp_drv.ver_res = screenHeight; disp_drv.flush_cb = my_disp_flush; disp_drv.draw_buf = &draw_buf; lv_disp_drv_register(&disp_drv);
    static lv_indev_drv_t indev_drv; lv_indev_drv_init(&indev_drv); indev_drv.type = LV_INDEV_TYPE_POINTER; indev_drv.read_cb = my_touchpad_read; lv_indev_drv_register(&indev_drv);

    build_settings_ui();
    build_main_ui();

    // WiFiManager stopped untill not connected, then we ar online!
    Serial.print("WiFi connected! IP address: ");
    Serial.println(WiFi.localIP());

    setup_web_server(); // Turn on web server

    lv_scr_load(main_screen);

    // --- Creation of the protective black shield ---
    touch_shield = lv_obj_create(lv_layer_sys()); // The system layer is more upper
    lv_obj_set_size(touch_shield, LV_PCT(100), LV_PCT(100)); // Cover all
    lv_obj_set_style_bg_color(touch_shield, lv_color_black(), 0); // Black screen
    lv_obj_set_style_bg_opa(touch_shield, LV_OPA_100, 0); // 100% Opacity
    lv_obj_set_style_border_width(touch_shield, 0, 0); // No border
    lv_obj_add_flag(touch_shield, LV_OBJ_FLAG_CLICKABLE); // Capture the touch
    lv_obj_add_event_cb(touch_shield, shield_event_cb, LV_EVENT_CLICKED, NULL); // Wakeup at touch
    lv_obj_add_flag(touch_shield, LV_OBJ_FLAG_HIDDEN); // default hide at boot
}

void loop() {
    lv_tick_inc(5);
    lv_timer_handler();
    delay(5);
    
    // --- Managing Black screen ---
    if (auto_screen_off) {
        if (lv_disp_get_inactive_time(NULL) > 6000000) { // 10 minutes
            if (!screen_is_off) {
                Serial.println(">>> TIME OUT: BLACK SHIELD ACTIVATED <<<");
                // digitalWrite(BACKLIGHT_PIN, HIGH); // Turn off bacl light (not working)
                if (touch_shield != NULL) lv_obj_clear_flag(touch_shield, LV_OBJ_FLAG_HIDDEN); // Show black
                screen_is_off = true;
            }
        }
    } else if (screen_is_off) {
        // digitalWrite(BACKLIGHT_PIN, LOW);
        if (touch_shield != NULL) lv_obj_add_flag(touch_shield, LV_OBJ_FLAG_HIDDEN);
        screen_is_off = false;
    }

    handle_web_server(); // Client listening in background
    handle_mqtt(); // Managing IN Connection and messages

    static unsigned long last_mqtt_pub = 0;
    if (millis() - last_mqtt_pub > 15000) { // Public MQTT every 15 sec
        publish_data();
        last_mqtt_pub = millis();
    }
    
    // --- Managing RGB led state ---
    static unsigned long last_blink = 0;
    static bool blink_state = false;

    if (is_running) {
        // Led Red Intermintent (when Heating)
        if (millis() - last_blink > 500) { // Intermintent every 500ms
            last_blink = millis();
            blink_state = !blink_state;
            
            // 240 low intesity. 255 is OFF. (Up to 0, Max intensity)
            if (blink_state) ledcWrite(3, 240); 
            else ledcWrite(3, 255);            
            
            ledcWrite(4, 255); // Green led OFF
        }
    } else {
        // Green led ON (when in standby)
        ledcWrite(3, 255); // Led Red off
        ledcWrite(4, 240); // Green led ON whit low intensity (240)
    }

    static uint32_t last_timer_tick = 0;
    static uint32_t last_sensor = 0;

    if (lv_scr_act() == main_screen) {
        if (is_running && (millis() - last_timer_tick >= 1000)) {
            if (remaining_seconds > 0) remaining_seconds--;
            else {
                is_running = false;
                ledcWrite(PWM_CH_HEATER, 0); ledcWrite(PWM_CH_FAN, 0);
                lv_label_set_text(label_btn_start, "START"); lv_obj_set_style_bg_color(btn_start, lv_color_hex(0x00AA00), 0);
                lv_label_set_text(label_status, "Status: DONE!"); lv_obj_set_style_text_color(label_status, lv_color_hex(0x00FF00), 0);
            }
            update_timer_label();
            last_timer_tick = millis();
        }

        if (millis() - last_sensor > 1000) {
            float t = sht31.readTemperature(); float h = sht31.readHumidity();
            if (!isnan(t)) {
                pid_Input = t;
                lv_label_set_text(label_temp, (String(t, 1) + " C").c_str());
                lv_label_set_text(label_hum, (String(h, 0) + " %").c_str());
                if (t >= pid_Setpoint) lv_obj_set_style_text_color(label_temp, lv_color_hex(0x00FF00), 0);
                else lv_obj_set_style_text_color(label_temp, lv_color_hex(0xFF3333), 0);

                if (is_running) {
                    boxPID.Compute(); ledcWrite(PWM_CH_HEATER, pid_Output);
                    int fan_pwm = map(pid_Output, 0, 255, 75, 180);
                    ledcWrite(PWM_CH_FAN, fan_pwm);
                    int heater_pct = (pid_Output / 255.0) * 100; lv_label_set_text_fmt(label_heater_per, "%d%%", heater_pct);
                    int fan_pct = (fan_pwm / 255.0) * 100; lv_label_set_text_fmt(label_fan_per, "%d%%", fan_pct);
                } else {
                    lv_label_set_text(label_heater_per, "0%"); lv_label_set_text(label_fan_per, "0%");
                }
            }
            last_sensor = millis();
        }
    }
}