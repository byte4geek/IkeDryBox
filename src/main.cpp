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
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// --- EFFECTIVE INSTANTIATION OF GLOBAL VARIABLES ---
Preferences prefs;

double pid_Setpoint = 50.0, pid_Input, pid_Output;
int current_humidity = 0; // Initialize at zero for fix NaN
double Kp = 60.0, Ki = 0.90, Kd = 8.0; // The best numbers to start the tuning
PID boxPID(&pid_Input, &pid_Output, &pid_Setpoint, Kp, Ki, Kd, DIRECT);

String hostname = "IkeDryBox"; // default name
String current_status = "Ready"; // For MQTT
lv_obj_t *ta_hostname;

bool is_running = false;
long remaining_seconds = 5 * 3600;
bool opt_in_telemetry = true;
long target_time_sec = 0;

bool use_dhcp = true; 
lv_obj_t *cb_dhcp;

int screen_timeout_mins = DEFAULT_SCREEN_TIMEOUT;

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

// --- LED intensity (0 = off, 255 = max) ---
int led_r_intensity = 10; // Red LED intensity (default barely visible)
int led_g_intensity = 10; // Green LED intensity

// --- Chart data points ---
int chart_max_points = 900; // Default 900 points (~30 min at 2s poll)

// --- Dynamic hardware selection variables ---
bool is_st7789 = true;
int PIN_HEATER = 19;
int PIN_FAN = 23;
int BACKLIGHT_PIN = 21;
int LED_R = 22;
int LED_G = 16;

// --- CONFIGURATION HARDWARE DISPLAY ---
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9341 _panel_ili;
    lgfx::Panel_ST7789 _panel_st;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Light_PWM _light_instance;
    lgfx::Touch_XPT2046 _touch_instance;
public:
    LGFX(void) {}

    void init_for_board(bool use_st7789) {
        { 
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 40000000;
            cfg.pin_sclk = 14;
            cfg.pin_mosi = 13;
            cfg.pin_miso = 12;
            cfg.pin_dc = 2;
            cfg.dma_channel = 0;
            _bus_instance.config(cfg);
        }
        { 
            auto cfg = _light_instance.config();
            cfg.pin_bl      = use_st7789 ? 21 : 27;
            cfg.invert      = false;
            cfg.freq        = 44100;
            cfg.pwm_channel = 7;
            _light_instance.config(cfg);
        }
        if (use_st7789) {
            auto cfg = _panel_st.config();
            cfg.pin_cs = 15;
            cfg.pin_rst = -1;
            cfg.pin_busy = -1;
            cfg.memory_width = 240;
            cfg.memory_height = 320;
            cfg.panel_width = 240;
            cfg.panel_height = 320;
            cfg.offset_rotation = 0;
            cfg.rgb_order = false;
            cfg.invert = false;
            _panel_st.config(cfg);
            _panel_st.setBus(&_bus_instance);
            _panel_st.setLight(&_light_instance);
            {
                auto cfg_t = _touch_instance.config();
                cfg_t.x_min = 222;
                cfg_t.x_max = 3367;
                cfg_t.y_min = 192;
                cfg_t.y_max = 3732;
                cfg_t.bus_shared = false;
                cfg_t.spi_host = SPI3_HOST;
                cfg_t.pin_sclk = 25;
                cfg_t.pin_mosi = 32;
                cfg_t.pin_miso = 39;
                cfg_t.pin_cs = 33;
                cfg_t.pin_int = 36;
                _touch_instance.config(cfg_t);
                _panel_st.setTouch(&_touch_instance);
            }
            setPanel(&_panel_st);
        } else {
            auto cfg = _panel_ili.config();
            cfg.pin_cs = 15;
            cfg.pin_rst = -1;
            cfg.pin_busy = -1;
            cfg.memory_width = 320;
            cfg.memory_height = 240;
            cfg.panel_width = 320;
            cfg.panel_height = 240;
            cfg.offset_rotation = 4;
            cfg.rgb_order = true;
            cfg.invert = false;
            _panel_ili.config(cfg);
            _panel_ili.setBus(&_bus_instance);
            _panel_ili.setLight(&_light_instance);
            {
                auto cfg_t = _touch_instance.config();
                cfg_t.x_min = 222;
                cfg_t.x_max = 3367;
                cfg_t.y_min = 192;
                cfg_t.y_max = 3732;
                cfg_t.bus_shared = true;
                cfg_t.spi_host = SPI2_HOST;
                cfg_t.pin_cs = 33;
                _touch_instance.config(cfg_t);
                _panel_ili.setTouch(&_touch_instance);
            }
            setPanel(&_panel_ili);
        }
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
    
    lv_disp_trig_activity(NULL); // <-- THE COMMAND TO RESET THE TIMER
    Serial.println(">>> SHIELD TOUCHED: SCREEN WOKEN UP <<<");
    
    // 1. Remove the black shield immediately
    lv_obj_add_flag(touch_shield, LV_OBJ_FLAG_HIDDEN); // Hides the black shield
    
    // 2. Force LVGL to redraw the UI before the fade
    lv_tick_inc(5);
    lv_timer_handler();  

    // --- GRADUAL FADE IN 1 SECONDS ---
    for (int b = 0; b <= 180; b++) {
        tft.setBrightness(b);
        lv_tick_inc(5);
        lv_timer_handler();
        delay(5); // 180 step * 5ms = ~900ms ≈ 1 sec
    }
    // -------------------------------------
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
        if (is_st7789) {
            data->point.x = constrain(map(touchY, 300, 3800, 0, 320), 0, 320);
            data->point.y = constrain(map(touchX, 300, 3800, 0, 240), 0, 240);
        } else {
            data->point.x = constrain(map(touchX, 300, 3800, 0, 320), 0, 320);
            data->point.y = constrain(map(touchY, 300, 3800, 0, 240), 0, 240);
        }
    } else { data->state = LV_INDEV_STATE_REL; }
}

void load_settings() {
    prefs.begin("drybox", false);

    // Load hardware version first
    is_st7789 = prefs.getBool("is_st7789", true);

    // Assign pin variables dynamically based on hardware type
    if (is_st7789) {
        PIN_HEATER = 19;
        PIN_FAN = 23;
        BACKLIGHT_PIN = 21;
        LED_R = 22;
        LED_G = 16;
    } else {
        PIN_HEATER = 5;
        PIN_FAN = 23;
        BACKLIGHT_PIN = 27;
        LED_R = 4;
        LED_G = 17;
    }

    hostname = prefs.getString("hostname", "IkeDryBox");
    Kp = prefs.getDouble("kp", 113.0);
    Ki = prefs.getDouble("ki", 0.80);
    Kd = prefs.getDouble("kd", 3.2);
    boxPID.SetTunings(Kp, Ki, Kd);
    use_dhcp = prefs.getBool("dhcp", true);
    custom_temp = prefs.getFloat("c_temp", 50.0);
    custom_time = prefs.getLong("c_time", 18000);
    screen_timeout_mins = prefs.getInt("scr_to", DEFAULT_SCREEN_TIMEOUT);
    auto_screen_off = prefs.getBool("scr_off", false);
    led_r_intensity = prefs.getInt("led_r", 10);
    led_g_intensity = prefs.getInt("led_g", 10);
    chart_max_points = prefs.getInt("chrt_pts", 900);
    opt_in_telemetry = prefs.getBool("telemetry", true);

    // If the IP is static, we configure WiFi
    if (!use_dhcp) {
        IPAddress ip, gw, nm;
        ip.fromString(prefs.getString("ip", "192.168.1.100"));
        gw.fromString(prefs.getString("gw", "192.168.1.1"));
        nm.fromString(prefs.getString("nm", "255.255.255.0"));
        WiFi.config(ip, gw, nm, gw, IPAddress(8, 8, 8, 8));
        WiFi.setHostname(hostname.c_str());
    }
    prefs.end();
}

void setup() {
    Serial.begin(115200);
    
    load_settings();
    
    // pinMode(21, OUTPUT); 
    // digitalWrite(21, LOW);
    pinMode(LED_G, OUTPUT); digitalWrite(LED_G, HIGH);
    pinMode(LED_R, OUTPUT);  digitalWrite(LED_R, HIGH);
    
    // pinMode(BACKLIGHT_PIN, OUTPUT);
    // digitalWrite(BACKLIGHT_PIN, LOW); // Turn on screen
    
    // Channel PWM 3 and 4 for the LEDs
    ledcSetup(3, 5000, 8); // Channel for Red
    ledcAttachPin(LED_R, 3);
    ledcSetup(4, 5000, 8); // Channel for Green
    ledcAttachPin(LED_G, 4);

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

    tft.init_for_board(is_st7789);
    tft.init();
    tft.setRotation(is_st7789 ? 1 : 0);
    tft.setBrightness(180); tft.fillScreen(0);
    tft.setCursor(20, 20); tft.setTextColor(0xFFAA00); tft.println("Booting IkeDryBox...");

    WiFiManager wm;
    wm.setHostname(hostname.c_str()); // send hostname to the router
    wm.setConfigPortalTimeout(180); // If it doesn't connect within 3 minutes, it exits and restarts

    // Call Autoconnect one time only
    // 1. We create a REAL variable that occupies a fixed space in memory
    String ap_name = hostname + "_Setup"; 
    
    // 2. We pass the key of this fixed variable to the WiFiManager
    if (!wm.autoConnect(ap_name.c_str())) {
        Serial.println("WiFi connection failed and timeout reached. Force restart...");
        delay(3000);
        ESP.restart();
    }
    
    // Start sensor (one time only)
    if (is_st7789) {
        Wire.begin(27, 18);
    } else {
        Wire.begin(21, 22);
    }
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

    uint32_t uptime_sec = (uint32_t)(esp_timer_get_time() / 1000000ULL);
    send_telemetry("none", 0.0, 0, 0, "boot", uptime_sec);
}

struct TelemetryPayload {
    String device_id;
    String filament;
    float target_temp;
    int target_time_sec;
    int actual_duration_sec;
    String status;
    uint32_t uptime_sec;
};

String get_telemetry_device_id() {
    prefs.begin("drybox", false);
    String id = prefs.getString("device_id", "");
    if (id == "") {
        id = "ibx_" + String(esp_random(), HEX);
        prefs.putString("device_id", id);
        Serial.println("[Telemetry] Generated new device ID: " + id);
    }
    prefs.end();
    return id;
}

void telemetry_task(void *pvParameters) {
    TelemetryPayload *payload = (TelemetryPayload *)pvParameters;
    if (payload != NULL) {
        WiFiClientSecure client;
        client.setInsecure();

        HTTPClient http;
        http.setTimeout(5000); // 5 seconds connection/write timeout

        if (http.begin(client, "https://ikb.byte4geek.workers.dev")) {
            http.addHeader("Content-Type", "application/json");
            http.addHeader("X-DryBox-Token", "SECRET_KEY_12345");

            JsonDocument doc;
            doc["device_id"] = payload->device_id;
            doc["filament"] = payload->filament;
            doc["target_temp"] = payload->target_temp;
            doc["target_time_minutes"] = payload->target_time_sec / 60;
            doc["actual_duration_minutes"] = payload->actual_duration_sec / 60;
            doc["status"] = payload->status;
            doc["uptime_seconds"] = payload->uptime_sec;
            doc["fw_version"] = FIRMWARE_VERSION;
            doc["display_type"] = is_st7789 ? "ST7789" : "ILI9341";

            String body;
            serializeJson(doc, body);

            Serial.println("[Telemetry Task] Sending JSON: " + body);
            int httpResponseCode = http.POST(body);
            if (httpResponseCode > 0) {
                Serial.printf("[Telemetry Task] HTTPS POST Success: %d\n", httpResponseCode);
            } else {
                Serial.printf("[Telemetry Task] HTTPS POST Error: %s\n", http.errorToString(httpResponseCode).c_str());
            }
            http.end();
        } else {
            Serial.println("[Telemetry Task] Unable to initialize HTTPClient");
        }
        delete payload;
    }
    vTaskDelete(NULL); // Auto-destroy the task
}

void send_telemetry(String filament_type, float target_temp, int target_time_sec, int actual_duration_sec, String status, uint32_t uptime_sec) {
    if (!opt_in_telemetry && status != "boot") {
        Serial.println("[Telemetry] Skipped (opt-out)");
        return;
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[Telemetry] Skipped (WiFi not connected)");
        return;
    }

    TelemetryPayload *payload = new TelemetryPayload();
    payload->device_id = get_telemetry_device_id();
    payload->filament = filament_type;
    payload->target_temp = target_temp;
    payload->target_time_sec = target_time_sec;
    payload->actual_duration_sec = actual_duration_sec;
    payload->status = status;
    payload->uptime_sec = uptime_sec;

    BaseType_t res = xTaskCreate(
        telemetry_task,
        "tele_task",
        8192,
        (void *)payload,
        1,
        NULL
    );
    if (res != pdPASS) {
        Serial.println("[Telemetry] Failed to create background task");
        delete payload;
    }
}

void loop() {
    static bool was_running = false;
    if (is_running && !was_running) {
        target_time_sec = remaining_seconds;
        was_running = true;
        Serial.printf("[Telemetry] Cycle started. Target time recorded: %ld sec\n", target_time_sec);
    } else if (!is_running && was_running) {
        uint32_t uptime_sec = (uint32_t)(esp_timer_get_time() / 1000000ULL);
        if (remaining_seconds <= 0) {
            // Finished naturally
            send_telemetry(current_filament, pid_Setpoint, target_time_sec, target_time_sec, "completed", uptime_sec);
        } else {
            // Manually stopped
            long actual_duration_sec = target_time_sec - remaining_seconds;
            if (actual_duration_sec < 0) actual_duration_sec = 0;
            if (actual_duration_sec > 60) {
                send_telemetry(current_filament, pid_Setpoint, target_time_sec, actual_duration_sec, "interrupted", uptime_sec);
            } else {
                Serial.printf("[Telemetry] Cycle interrupted after %ld sec (<= 60s, skipped)\n", actual_duration_sec);
            }
        }
        was_running = false;
    }

    lv_tick_inc(5);
    lv_timer_handler();
    delay(5);
    
    // --- Managing Black screen/back light ---
    if (auto_screen_off && screen_timeout_mins > 0) { // If enabled and timeout > 0
        
        // We convert minutes to milliseconds (1 min = 60,000 ms) 
        uint32_t timeout_ms = (uint32_t)screen_timeout_mins * 60000; 
        
        if (lv_disp_get_inactive_time(NULL) > timeout_ms) { // managed by webui default 10 mins substitute variable with number (10000) to test it
            if (!screen_is_off) {
                Serial.println(">>> TIME OUT: BLACK SHIELD ACTIVATED <<<");

                // --- GRADUAL FADE OUT IN 3 SECONDS ---
                // 180 steps of 1, each step = 3000ms / 180 = ~16ms
                for (int b = 180; b >= 0; b--) {
                    tft.setBrightness(b);
                    lv_tick_inc(16);
                    lv_timer_handler();
                    delay(16);
                }
                // ---------------------------------------

                if (touch_shield != NULL) lv_obj_clear_flag(touch_shield, LV_OBJ_FLAG_HIDDEN); // Show black
                screen_is_off = true;
            }
        }
    } else if (screen_is_off || !auto_screen_off) {
        // If the screen saver was active but has been disabled by settings 
        // or if screen_timeout_mins has become 0, we turn the light back to maximum 
        tft.setBrightness(180); // turn on the light
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
        // Red LED blinking (when heating)
        if (millis() - last_blink > 500) { // Blink every 500ms
            last_blink = millis();
            blink_state = !blink_state;
            
            if (blink_state) ledcWrite(3, 255 - led_r_intensity);
            else ledcWrite(3, 255);
            
            ledcWrite(4, 255); // Green LED off
        }
    } else {
        // Green LED on (when in standby)
        ledcWrite(3, 255); // Red LED off
        ledcWrite(4, 255 - led_g_intensity); // Green LED on with variable intensity
    }

    static uint32_t last_timer_tick = 0;
    static uint32_t last_sensor = 0;

    if (lv_scr_act() == main_screen) {
        if (is_running && (millis() - last_timer_tick >= 1000)) {
            if (remaining_seconds > 0) remaining_seconds--;
            else {
                is_running = false;
                current_status = "Done"; // added this for mqtt status
                ledcWrite(PWM_CH_HEATER, 0); ledcWrite(PWM_CH_FAN, 0);
                lv_label_set_text(label_btn_start, "START"); lv_obj_set_style_bg_color(btn_start, lv_color_hex(0x00AA00), 0);
                lv_label_set_text(label_status, "Status: DONE!"); lv_obj_set_style_text_color(label_status, lv_color_hex(0x00FF00), 0);
            }
            update_timer_label();
            last_timer_tick = millis();
        }

        if (millis() - last_sensor > 1000) {
            float t = sht31.readTemperature(); 
            float h = sht31.readHumidity();
            
            if (!isnan(t)) {
                pid_Input = t;
                lv_label_set_text(label_temp, (String(t, 1) + " C").c_str());
                
                if (t >= pid_Setpoint) lv_obj_set_style_text_color(label_temp, lv_color_hex(0x00FF00), 0);
                else lv_obj_set_style_text_color(label_temp, lv_color_hex(0xFF3333), 0);

                // --- Secure humidity managing and crentralized ---
                if (!isnan(h)) {
                    current_humidity = (int)h; // Save the value only if it's valid
                }
                lv_label_set_text(label_hum, (String(current_humidity) + " %").c_str());
                // ------------------------------------------------

                if (is_running) {
                    boxPID.Compute(); ledcWrite(PWM_CH_HEATER, pid_Output);
                    int fan_pwm = map(pid_Output, 0, 255, 75, 255);
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