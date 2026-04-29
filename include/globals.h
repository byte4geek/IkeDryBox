// File: include/globals.h
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <lvgl.h>
#include <PID_v1.h>
#include <Preferences.h>
#include <Adafruit_SHT31.h>
#include <PubSubClient.h>

// --- PIN and HARDWARE ---
#define PIN_HEATER 5
#define PIN_FAN 23
#define PWM_CH_HEATER 0
#define PWM_CH_FAN 1
// #define PWM_FREQ 25000 // 25 kHz - Beyond the threshold of human hearing (not working)
#define HEATER_FREQ 1000 // Safe frequency for heater MOSFET, makes no noise
#define FAN_FREQ 100      // Ultra-low frequency (Plan B to remove the whistle)
#define PWM_RES 8

// Display backlight PIN
#define BACKLIGHT_PIN 27
#define PWM_CH_BACKLIGHT 2

// Led RGB
#define LED_R 4
#define LED_B 17

// --- GLOBAL INSTANCES ---
extern Preferences prefs;
extern PID boxPID;
extern Adafruit_SHT31 sht31;

// --- Touch_shield at screen off
extern lv_obj_t * touch_shield;

// --- VARIABILES PID AND STATE ---
extern double pid_Setpoint;
extern double pid_Input;
extern double pid_Output;
extern double Kp, Ki, Kd;
extern bool is_running;
extern long remaining_seconds;

// --- hostname ---
extern String hostname;
extern lv_obj_t *ta_hostname;

// --- DHCP ---
extern bool use_dhcp;
extern lv_obj_t *cb_dhcp; // The checkbox for the display

// --- Screen and UI shared objects ---
extern lv_obj_t *main_screen;
extern lv_obj_t *settings_screen;

// Objects that main.cpp must update (Sensors, timers, etc.)
extern lv_obj_t *label_temp; // temperature
extern lv_obj_t *label_hum; // humidity
extern lv_obj_t *label_heater_per; 
extern lv_obj_t *label_fan_per;
extern lv_obj_t *label_status;
extern lv_obj_t *btn_start;
extern lv_obj_t *label_btn_start;
extern lv_obj_t *label_timer;
extern lv_obj_t *label_target;
extern lv_obj_t *ta_ip, *ta_gw, *ta_nm; // Network
extern lv_obj_t *ta_mqtt_host, *ta_mqtt_port, *ta_mqtt_user, *ta_mqtt_pass; // MQTT

extern WiFiClient espClient;
extern PubSubClient mqttClient;
extern String current_filament; // To trace the selected material
extern void publish_data();

// --- CUSTOM FILAMENT ---
extern float custom_temp;
extern long custom_time;
extern lv_obj_t *slider_c_temp;
extern lv_obj_t *slider_c_time;
extern lv_obj_t *lbl_val_c_temp;
extern lv_obj_t *lbl_val_c_time;

// --- Backlight ---
extern bool auto_screen_off;
extern bool screen_is_off;