// File: src/mqtt_manager.cpp
#include "globals.h"
#include "mqtt_manager.h"
#include <ArduinoJson.h>
#include "ui_main.h" 

WiFiClient espClient;
PubSubClient mqttClient(espClient);
String current_filament = "PLA";

static String mqtt_host_cache = "";
static bool mqtt_cache_valid = false; // false = re-read from prefs on next handle_mqtt()

String get_device_id() {
    String id = hostname;
    id.replace(" ", "_");
    id.toLowerCase();
    return id;
}

void add_device_info(JsonObject& dev, String device_id) {
    dev["ids"] = device_id;
    dev["name"] = hostname; 
    dev["mf"] = "PeppeBytes"; 
    dev["mdl"] = String("v") + FIRMWARE_VERSION; // Centralized firmware version fron globals.h
}

// --- THE STREAMING HELPER (Bypasses 256 byte limits!) ---
void publish_discovery_packet(String topic, String payload) {
    // 1. We open the channel by saying how long the message will be (true = retained)
    if (mqttClient.beginPublish(topic.c_str(), payload.length(), true)) {
        // 2. Let's flood the channel with our giant JSON
        mqttClient.print(payload);
        // 3. We close and send it
        mqttClient.endPublish();
        Serial.print("[MQTT Discovery] Sent OK: ");
    } else {
        Serial.print("[MQTT Discovery] ERROR (Channel not opened): ");
    }
    Serial.println(topic);
    
    mqttClient.loop(); 
    delay(50); // Very small pause to avoid clogging up the router
}

void send_ha_discovery() {
    String base = "homeassistant/";
    String dev_id = get_device_id();
    String stat_t = dev_id + "/state";
    String cmd_base = dev_id + "/cmd/";
    
    Serial.println("\n--- START SENDING AUTO-DISCOVERY HA ---");

    // 1. Temperature sensor
    {
        JsonDocument doc;
        doc["name"] = "Temperature";
        doc["stat_t"] = stat_t;
        doc["unit_of_meas"] = "°C";
        doc["val_tpl"] = "{{ value_json.temp }}";
        doc["dev_cla"] = "temperature";
        doc["uniq_id"] = dev_id + "_temp";
        JsonObject dev = doc["dev"].to<JsonObject>();
        add_device_info(dev, dev_id);
        String out; serializeJson(doc, out);
        publish_discovery_packet(base + "sensor/" + dev_id + "/temp/config", out);
    }
    
    // 2. Humidity sensor
    {
        JsonDocument doc;
        doc["name"] = "Humidity";
        doc["stat_t"] = stat_t;
        doc["unit_of_meas"] = "%";
        doc["val_tpl"] = "{{ value_json.hum }}";
        doc["dev_cla"] = "humidity";
        doc["uniq_id"] = dev_id + "_hum";
        JsonObject dev = doc["dev"].to<JsonObject>();
        add_device_info(dev, dev_id);
        String out; serializeJson(doc, out);
        publish_discovery_packet(base + "sensor/" + dev_id + "/hum/config", out);
    }
    
    // 3. Switch Start/Stop
    {
        JsonDocument doc;
        doc["name"] = "Process";
        doc["stat_t"] = stat_t;
        doc["val_tpl"] = "{{ 'ON' if value_json.running else 'OFF' }}";
        doc["cmd_t"] = cmd_base + "switch";
        doc["uniq_id"] = dev_id + "_switch";
        JsonObject dev = doc["dev"].to<JsonObject>();
        add_device_info(dev, dev_id);
        String out; serializeJson(doc, out);
        publish_discovery_packet(base + "switch/" + dev_id + "/power/config", out);
    }

    // 4. Temperature Target Selector
    {
        JsonDocument doc;
        doc["name"] = "Target Temp";
        doc["stat_t"] = stat_t;
        doc["val_tpl"] = "{{ value_json.target }}";
        doc["cmd_t"] = cmd_base + "target";
        doc["unit_of_meas"] = "°C";
        doc["icon"] = "mdi:thermometer"; // Thermometer icon on HA
        doc["min"] = 20;
        doc["max"] = 90;
        doc["step"] = 1;
        doc["uniq_id"] = dev_id + "_target";
        JsonObject dev = doc["dev"].to<JsonObject>();
        add_device_info(dev, dev_id);
        String out; serializeJson(doc, out);
        publish_discovery_packet(base + "number/" + dev_id + "/target/config", out);
    }

    // 5. Filament Select (It replace the older readonly sensor)
    {
         JsonDocument doc;
         doc["name"] = "Filament";
         doc["stat_t"] = stat_t;
         doc["cmd_t"] = cmd_base + "filament"; // Riceve i comandi da HA
         doc["val_tpl"] = "{{ value_json.filament }}";
         doc["icon"] = "mdi:movie-roll";
         JsonArray options = doc["options"].to<JsonArray>();
         options.add("PLA");
         options.add("PETG");
         options.add("ABS");
         options.add("TPU");
         // options.add("Nylon"); //not safe for Ikea box
         options.add("Custom");
         doc["uniq_id"] = dev_id + "_fil_select"; 
         JsonObject dev = doc["dev"].to<JsonObject>();
         add_device_info(dev, dev_id);
         String out; serializeJson(doc, out);
         publish_discovery_packet(base + "select/" + dev_id + "/filament/config", out);
    }

    // 6. Sensor Timer (Time Remaining)
    {
         JsonDocument doc;
         doc["name"] = "Time Remaining";
         doc["stat_t"] = stat_t;
         doc["val_tpl"] = "{{ value_json.time_str }}";
         doc["icon"] = "mdi:timer-outline"; // Puts the stopwatch icon on HA!
         doc["uniq_id"] = dev_id + "_timer";
         JsonObject dev = doc["dev"].to<JsonObject>();
         add_device_info(dev, dev_id);
         String out; serializeJson(doc, out);
         publish_discovery_packet(base + "sensor/" + dev_id + "/timer/config", out);
    }

    // 7. Sensor Heater Power %
    {
         JsonDocument doc;
         doc["name"] = "Heater Power";
         doc["stat_t"] = stat_t;
         doc["unit_of_meas"] = "%";
         doc["val_tpl"] = "{{ value_json.heater }}";
         doc["icon"] = "mdi:heating-coil";
         doc["uniq_id"] = dev_id + "_heater";
         JsonObject dev = doc["dev"].to<JsonObject>();
         add_device_info(dev, dev_id);
         String out; serializeJson(doc, out);
         publish_discovery_packet(base + "sensor/" + dev_id + "/heater/config", out);
    }

    // 8. Sensor Fan Speed %
    {
         JsonDocument doc;
         doc["name"] = "Fan Speed";
         doc["stat_t"] = stat_t;
         doc["unit_of_meas"] = "%";
         doc["val_tpl"] = "{{ value_json.fan }}";
         doc["icon"] = "mdi:fan";
         doc["uniq_id"] = dev_id + "_fan";
         JsonObject dev = doc["dev"].to<JsonObject>();
         add_device_info(dev, dev_id);
         String out; serializeJson(doc, out);
         publish_discovery_packet(base + "sensor/" + dev_id + "/fan/config", out);
    }

    // 9. Sensor Status (Ready/Drying/Done)
    {
         JsonDocument doc;
         doc["name"] = "Status";
         doc["stat_t"] = stat_t;
         doc["val_tpl"] = "{{ value_json.status }}";
         doc["icon"] = "mdi:information-outline";
         doc["uniq_id"] = dev_id + "_status";
         JsonObject dev = doc["dev"].to<JsonObject>();
         add_device_info(dev, dev_id);
         String out; serializeJson(doc, out);
         publish_discovery_packet(base + "sensor/" + dev_id + "/status/config", out);
    }

    Serial.println("--- ENDS SEND AUTO-DISCOVERY HA ---\n");
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String t = String(topic);
    String msg = "";
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
    
    String dev_id = get_device_id();

    if (t == (dev_id + "/cmd/switch")) {
        bool turn_on = (msg == "ON");
        if (turn_on != is_running) {
            is_running = turn_on;
            if(is_running) {
                current_status = "Drying"; // <-- State update
                lv_label_set_text(label_btn_start, "STOP");
                lv_obj_set_style_bg_color(btn_start, lv_color_hex(0xCC0000), 0);
                ledcWrite(PWM_CH_FAN, 255);
            } else {
                current_status = "Ready"; // <-- State update
                lv_label_set_text(label_btn_start, "START");
                lv_obj_set_style_bg_color(btn_start, lv_color_hex(0x00AA00), 0);
                ledcWrite(PWM_CH_HEATER, 0); ledcWrite(PWM_CH_FAN, 0);
            }
            publish_data();
        }
    }
    else if (t == (dev_id + "/cmd/target")) {
        float val = msg.toFloat();
        if (val >= 20.0 && val <= 90.0) {
            pid_Setpoint = val;
            lv_label_set_text_fmt(label_target, "Target: %d C", (int)pid_Setpoint);
            publish_data(); 
        }
    }
    else if (t == (dev_id + "/cmd/filament")) {
        // Receiving the command from the HA drop-down menu
        current_filament = msg;

        // --- START NEW LOGIC: Associate temperature AND timer ---
        if (msg == "PLA") {
            pid_Setpoint = TEMP_PLA;
            remaining_seconds = TIME_PLA;
        }
        else if (msg == "PETG") {
            pid_Setpoint = TEMP_PETG;
            remaining_seconds = TIME_PETG;
        }
        else if (msg == "ABS") {
            pid_Setpoint = TEMP_ABS;
            remaining_seconds = TIME_ABS;
        }
        else if (msg == "TPU") {
            pid_Setpoint = TEMP_TPU;
            remaining_seconds = TIME_TPU;
        }
        else if (msg == "Custom") {
            pid_Setpoint = custom_temp;
            remaining_seconds = custom_time; 
        }
        
        // Update the temperature on the physical LCD display
        lv_label_set_text_fmt(label_target, "Target: %d C", (int)pid_Setpoint);
        
        // Force the timer to update graphically on the display
        update_timer_label();
        // --- ENDS NEW LOGIC ---

        // Publish the data (Home Assistant will now receive the new target and timer!)
        publish_data();
    }
}

void setup_mqtt() {
    // No need to force the buffer anymore, we manage large packets in streaming!
}

void handle_mqtt() {
    static int port = 1883;
    static String user = "";
    static String pass = "";

    // When mqtt_cache_valid becomes false (after save), we reload credentials
    if (!mqtt_cache_valid) {
        mqttClient.disconnect();
        prefs.begin("drybox", true);
        // We use a safe check: if it doesn't exist, we put an empty string and it gives no error
        mqtt_host_cache = prefs.isKey("mqtt") ? prefs.getString("mqtt", "") : "";
        port = prefs.isKey("m_port") ? prefs.getString("m_port", "1883").toInt() : 1883;
        user = prefs.isKey("m_user") ? prefs.getString("m_user", "") : "";
        pass = prefs.isKey("m_pass") ? prefs.getString("m_pass", "") : "";
        prefs.end();
        mqtt_cache_valid = true;
    }

    if (mqtt_host_cache == "") return; // If the host is empty, it does nothing and does NOT clog up the Serial

    if (!mqttClient.connected()) {
        static unsigned long last_reconnect_attempt = 0;
        
        if (millis() - last_reconnect_attempt > 5000) { 
            last_reconnect_attempt = millis();
            
            mqttClient.setServer(mqtt_host_cache.c_str(), port);
            mqttClient.setCallback(mqtt_callback);

            String dev_id = get_device_id();
            bool connected = false;
            
            if(user != "") connected = mqttClient.connect(dev_id.c_str(), user.c_str(), pass.c_str());
            else connected = mqttClient.connect(dev_id.c_str());

            if (connected) {
                Serial.println("\n[MQTT] Connected to the Broker whit success!");
                send_ha_discovery();
                mqttClient.subscribe((dev_id + "/cmd/#").c_str()); 
            } else {
                Serial.print("\n[MQTT] Connction failed. State code (rc): ");
                Serial.println(mqttClient.state());
            }
        }
    } else {
        mqttClient.loop();
    }
}

void publish_data() {
    if (!mqttClient.connected()) return;
    
    JsonDocument doc;
    doc["temp"] = pid_Input;
    doc["hum"] = current_humidity; // <-- Read global variable fix for NaN
    doc["running"] = is_running;
    doc["filament"] = current_filament;
    doc["target"] = pid_Setpoint;
    // send the status into JSON
    // --- CALCULATING STATE ---
    if (is_running) {
        current_status = "Drying";
    } else if (remaining_seconds <= 0) {
        current_status = "Done";
    } else {
        current_status = "Ready";
    }
    doc["status"] = current_status;
    // ---------

    // --- ADDED TIMER FOR MQTT ---
    int h = remaining_seconds / 3600;
    int m = (remaining_seconds % 3600) / 60;
    int s = remaining_seconds % 60;
    char t_buf[16]; 
    sprintf(t_buf, "%d:%02d:%02d", h, m, s);
    doc["time_str"] = t_buf;
    // -------------------------------
    
    if (is_running) {
        doc["heater"] = (int)((pid_Output / 255.0) * 100);
        int fan_pwm = map(pid_Output, 0, 255, 75, 255);
        doc["fan"] = (int)((fan_pwm / 255.0) * 100);
    } else {
        doc["heater"] = 0;
        doc["fan"] = 0;
    }
    
    String out;
    serializeJson(doc, out);
    
    String stat_t = get_device_id() + "/state";
    // Now we also send the status without stressing the buffer!
    if (mqttClient.beginPublish(stat_t.c_str(), out.length(), false)) {
        mqttClient.print(out);
        mqttClient.endPublish();
    }
}

void mqtt_force_reconnect() {
    mqtt_cache_valid = false;
}