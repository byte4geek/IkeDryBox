// File: src/web_server.cpp
#include "globals.h"
#include "web_server.h"
#include "ui_main.h"
#include <WebServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Update.h>

WebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>IkeDryBox Smart</title>
    <style>
        body { font-family: sans-serif; background-color: #121212; color: #fff; text-align: center; padding: 20px; max-width: 650px; margin: 0 auto; }
        .card { background: #1e1e1e; border-radius: 12px; padding: 20px; margin-bottom: 20px; box-shadow: 0 4px 10px rgba(0,0,0,0.5); }
        h1 { color: #FFAA00; margin-bottom: 30px; }
        .data { font-size: 2.5em; font-weight: bold; transition: color 0.3s; }
        .temp-default { color: #FF3333; } 
        .temp-ok { color: #00FF00; }
        .hum { color: #00BFFF; }
        .controls { display: flex; justify-content: center; align-items: center; gap: 15px; margin: 10px 0; }
        button { background: #333; color: white; border: 1px solid #555; padding: 10px 15px; border-radius: 8px; cursor: pointer; font-size: 1em; }
        .btn-main { background: #0088FF; width: 100%; font-size: 1.2em; padding: 15px; border: none; }
        .btn-start { background: #00AA00; } .btn-stop { background: #CC0000; }
        
        .panel { display: none; text-align: left; background: #252525; padding: 15px; border-radius: 8px; margin-top: 10px; }
        
        .input-group { margin-bottom: 10px; display: flex; justify-content: space-between; align-items: center;}
        input { background: #444; border: 1px solid #666; color: white; padding: 5px; border-radius: 4px; width: 150px; }
        input:disabled { background: #222; color: #888; border-color: #333; }
        input[type="file"] { width: 100%; padding: 10px; box-sizing: border-box; }
        
        .val-label { font-size: 1.5em; font-weight: bold; min-width: 80px; }
        .sub-text { margin-top:15px; color:#888; font-size: 0.9em; }
        
        .info-table { width: 100%; border-collapse: collapse; font-size: 0.95em; }
        .info-table td { padding: 8px 4px; border-bottom: 1px solid #333; }
        .info-table td:first-child { font-weight: bold; color: #aaa; width: 45%; }
    </style>
</head>
<body>
    <h1>IkeDryBox Dashboard</h1>

    <div class="card">
        <div style="display:flex; justify-content: space-around; align-items: center;">
            <div>
                <div class="data temp-default" id="t">--.-</div>
                <div style="font-size: 0.8em; color: #888;">CURRENT (&deg;C)</div>
            </div>
            <div>
                <div class="data hum" id="h">--</div>
                <div style="font-size: 0.8em; color: #888;">HUMIDITY (%)</div>
            </div>
        </div>
        <div class="sub-text">Heater: <span id="hp">0</span>% | Fan: <span id="fp">0</span>%</div>
    </div>

    <div class="card">
        <div style="color:#aaa; font-size: 0.9em; margin-bottom: 5px;">TARGET TEMPERATURE</div>
        <div class="controls">
            <button onclick="adjTarget(-1)">- 1&deg;</button>
            <div class="val-label" id="target_val">-- &deg;C</div>
            <button onclick="adjTarget(1)">+ 1&deg;</button>
        </div>
        
        <div class="input-group" style="justify-content: center; margin-top: 15px;">
            <label style="margin-right: 10px; color:#aaa;">Filament:</label>
            <select id="fil_sel" onchange="setFilament(this.value)" style="background:#444; color:white; padding:8px; border-radius:4px; font-size: 1.1em;">
                <option value="PLA">PLA</option>
                <option value="PETG">PETG</option>
                <option value="ABS">ABS</option>
                <option value="TPU">TPU</option>
                <option value="Custom">Custom</option>
            </select>
        </div>

        <hr style="border: 0; border-top: 1px solid #333; margin: 20px 0;">
        
        <div id="stat" style="font-weight:bold; color:#aaa; margin-bottom: 5px;">READY</div>
        <div class="controls">
            <button onclick="adjTime(-3600)">- 1h</button>
            <div class="val-label" id="tm">0:00:00</div>
            <button onclick="adjTime(3600)">+ 1h</button>
        </div>
        <button id="mainBtn" class="btn-main btn-start" style="margin-top:15px;" onclick="toggle()">START SESSION</button>
    </div>

    <div class="card">
        <div style="display: flex; gap: 10px;">
            <button class="btn-main" style="background:#444;" onclick="toggleConfig()">SETTINGS</button>
            <button class="btn-main" style="background:#444;" onclick="toggleInfo()">INFO</button>
        </div>
        
        <div id="config" class="panel">
            <h3 style="color:#00FF00; margin-top: 5px;">Custom Profile</h3>
            <div class="input-group"><label>Target Temp (&deg;C):</label><input type="number" id="c_temp" min="20" max="90"></div>
            <div class="input-group"><label>Timer (Hours):</label><input type="number" id="c_time" min="1" max="24"></div>
            <hr style="border: 0; border-top: 1px solid #333; margin: 25px 0;">

            <h3 style="color:#FFAA00">PID Tuning</h3>
            <div class="input-group"><label>Kp:</label><input type="number" id="kp"></div>
            <div class="input-group"><label>Ki:</label><input type="number" step="0.01" id="ki"></div>
            <div class="input-group"><label>Kd:</label><input type="number" step="0.1" id="kd"></div>
            
            <h3 style="color:#00BFFF">System, Network & MQTT</h3>
            <div class="input-group"><label>Auto Screen Off (10m):</label><input type="checkbox" id="scr_off" style="width:20px; height:20px;"></div>
            <div class="input-group"><label>Hostname:</label><input type="text" id="hostname"></div>
            <div class="input-group"><label>Use DHCP:</label><input type="checkbox" id="dhcp" onchange="toggleDhcp(this.checked)" style="width:20px; height:20px;"></div>
            <div class="input-group"><label>Static IP:</label><input type="text" id="ip"></div>
            <div class="input-group"><label>Gateway:</label><input type="text" id="gw"></div>
            <div class="input-group"><label>Netmask:</label><input type="text" id="nm"></div>
            <div class="input-group"><label>MQTT Broker:</label><input type="text" id="mq"></div>
            <div class="input-group"><label>MQTT Port:</label><input type="number" id="m_port"></div>
            <div class="input-group"><label>MQTT User:</label><input type="text" id="m_user"></div>
            <div class="input-group"><label>MQTT Pass:</label><input type="password" id="m_pass"></div>
            
            <button class="btn-main" style="margin-top: 15px;" onclick="saveConfig()">SAVE & APPLY</button>
            
            <hr style="border: 0; border-top: 1px solid #333; margin: 25px 0;">
            
            <h3 style="color:#FF3333">OTA Update</h3>
            <div style="background: #1a1a1a; padding: 10px; border-radius: 5px;">
                <input type="file" id="fw_file" accept=".bin" style="margin-bottom: 10px;">
                <button class="btn-main" style="background: #CC0000; padding: 10px;" onclick="uploadOTA()">FLASH FIRMWARE</button>
                <div id="ota_prg" style="color:#aaa; font-size:1.1em; font-weight:bold; margin-top:10px; text-align:center;"></div>
            </div>
        </div>

        <div id="info_panel" class="panel">
            <h3 style="color:#00BFFF; margin-top: 0;">Information</h3>
            <table class="info-table" id="info_table"></table>
        </div>
    </div>

    <script>
        function setFilament(val) { fetch('/api/set_filament?val=' + val, {method: 'POST'}).then(update); }
        
        function update() {
            fetch('/api/status').then(r => r.json()).then(d => {
                const tElem = document.getElementById('t');
                tElem.innerHTML = d.temp.toFixed(1);
                
                if(d.temp >= d.target) { tElem.className = "data temp-ok"; } 
                else { tElem.className = "data temp-default"; }

                document.getElementById('h').innerText = d.hum;
                document.getElementById('hp').innerText = d.heater;
                document.getElementById('fp').innerText = d.fan;
                document.getElementById('tm').innerText = d.time_str;
                document.getElementById('target_val').innerText = d.target.toFixed(0) + " \u00B0C";
                
                // Mantiene il dropdown sincronizzato con eventuali cambi fatti da schermo fisico
                let sel = document.getElementById('fil_sel');
                if(sel.value !== d.filament) sel.value = d.filament;

                let btn = document.getElementById('mainBtn');
                let stat = document.getElementById('stat');
                if(d.running) {
                    btn.innerText = "STOP SESSION"; btn.className = "btn-main btn-stop";
                    stat.innerText = "DRYING..."; stat.style.color = "#FFAA00";
                } else {
                    btn.innerText = "START SESSION"; btn.className = "btn-main btn-start";
                    stat.innerText = "READY"; stat.style.color = "#aaa";
                }
            });
        }

        function toggle() { fetch('/api/toggle', {method:'POST'}).then(update); }
        function adjTime(s) { fetch('/api/adj_time?val='+s, {method:'POST'}).then(update); }
        function adjTarget(v) { fetch('/api/adj_target?val='+v, {method:'POST'}).then(update); }
        
        function toggleDhcp(checked) {
            document.getElementById('ip').disabled = checked;
            document.getElementById('gw').disabled = checked;
            document.getElementById('nm').disabled = checked;
        }

        function toggleConfig() { 
            let p = document.getElementById('config');
            let info = document.getElementById('info_panel');
            info.style.display = 'none';

            p.style.display = (p.style.display == 'block') ? 'none' : 'block';
            if(p.style.display == 'block') {
                fetch('/api/config').then(r => r.json()).then(c => {
                    document.getElementById('c_temp').value = c.c_temp;
                    document.getElementById('c_time').value = c.c_time;
                    document.getElementById('hostname').value = c.hostname;
                    document.getElementById('kp').value = c.kp;
                    document.getElementById('ki').value = c.ki;
                    document.getElementById('kd').value = c.kd;
                    document.getElementById('dhcp').checked = c.dhcp;
                    document.getElementById('ip').value = c.ip;
                    document.getElementById('gw').value = c.gw;
                    document.getElementById('nm').value = c.nm;
                    document.getElementById('mq').value = c.mqtt;
                    document.getElementById('m_port').value = c.m_port;
                    document.getElementById('m_user').value = c.m_user;
                    document.getElementById('m_pass').value = c.m_pass;
                    document.getElementById('scr_off').checked = c.scr_off;
                    toggleDhcp(c.dhcp);
                });
            }
        }

        function toggleInfo() {
            let info = document.getElementById('info_panel');
            let conf = document.getElementById('config');
            conf.style.display = 'none';

            info.style.display = (info.style.display == 'block') ? 'none' : 'block';
            if(info.style.display == 'block') {
                fetch('/api/info').then(r => r.json()).then(d => {
                    let html = '';
                    const addRow = (k, v) => html += `<tr><td>${k}</td><td>${v}</td></tr>`;
                    
                    addRow("Program Version", d.fw_version);
                    addRow("Core/SDK Version", d.sdk);
                    addRow("ESP Chip Model", d.chip);
                    addRow("Uptime", d.uptime);
                    html += `<tr><td colspan="2"><hr style="border:0; border-top:1px solid #333; margin:5px 0;"></td></tr>`;
                    addRow("Wi-Fi SSID", d.ssid);
                    addRow("Wi-Fi RSSI", d.rssi);
                    addRow("MAC Address", d.mac);
                    addRow("IP Address", d.ip);
                    addRow("Gateway", d.gw);
                    addRow("Subnet Mask", d.nm);
                    html += `<tr><td colspan="2"><hr style="border:0; border-top:1px solid #333; margin:5px 0;"></td></tr>`;
                    addRow("Flash Size", d.flash_size);
                    addRow("Program Size", d.sketch_size);
                    addRow("Free Prg Space", d.free_sketch);
                    addRow("Free Memory", d.free_heap);

                    document.getElementById('info_table').innerHTML = html;
                });
            }
        }

        function saveConfig() {
            let data = {
                scr_off: document.getElementById('scr_off').checked,
                c_temp: parseFloat(document.getElementById('c_temp').value),
                c_time: parseInt(document.getElementById('c_time').value),
                hostname: document.getElementById('hostname').value,
                kp: parseFloat(document.getElementById('kp').value),
                ki: parseFloat(document.getElementById('ki').value),
                kd: parseFloat(document.getElementById('kd').value),
                dhcp: document.getElementById('dhcp').checked,
                ip: document.getElementById('ip').value,
                gw: document.getElementById('gw').value,
                nm: document.getElementById('nm').value,
                mqtt: document.getElementById('mq').value,
                m_port: document.getElementById('m_port').value,
                m_user: document.getElementById('m_user').value,
                m_pass: document.getElementById('m_pass').value
            };
            fetch('/api/save_config', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify(data)
            }).then(() => alert("Settings Saved! System will reboot."));
        }

        function uploadOTA() {
            let fileInput = document.getElementById('fw_file');
            let file = fileInput.files[0];
            if (!file) { alert("Please select a .bin file first!"); return; }
            
            let prg = document.getElementById('ota_prg');
            prg.innerText = "Starting upload...";
            
            let fd = new FormData();
            fd.append("update", file);
            
            let req = new XMLHttpRequest();
            req.open("POST", "/api/ota");
            
            req.upload.addEventListener("progress", e => {
                let p = Math.round((e.loaded / e.total) * 100);
                prg.innerText = "Progress: " + p + "%";
            });
            
            req.onreadystatechange = function() {
                if(req.readyState === 4) {
                    if(req.status === 200) {
                        prg.style.color = "#00FF00";
                        prg.innerText = "Update Successful! Rebooting...";
                        setTimeout(() => location.reload(), 8000);
                    } else {
                        prg.style.color = "#FF3333";
                        prg.innerText = "Error during update!";
                    }
                }
            };
            req.send(fd);
        }

        setInterval(update, 2000);
        update();
    </script>
</body>
</html>
)rawliteral";

void setup_web_server() {
    server.on("/", HTTP_GET, []() { server.send(200, "text/html", index_html); });

    server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });

    server.on("/api/status", HTTP_GET, []() {
        JsonDocument doc;
        doc["temp"] = pid_Input;
        doc["hum"] = (int)sht31.readHumidity();
        doc["target"] = pid_Setpoint;
        doc["filament"] = current_filament; // added to sync UI
        
        if (is_running) {
            doc["heater"] = (int)((pid_Output / 255.0) * 100);
            int fan_pwm = map(pid_Output, 0, 255, 75, 255);
            doc["fan"] = (int)((fan_pwm / 255.0) * 100);
        } else {
            doc["heater"] = 0;
            doc["fan"] = 0;
        }
        
        doc["running"] = is_running;
        
        int h = remaining_seconds / 3600;
        int m = (remaining_seconds % 3600) / 60;
        int s = remaining_seconds % 60;
        char t_buf[16]; sprintf(t_buf, "%d:%02d:%02d", h, m, s);
        doc["time_str"] = t_buf;

        String res; serializeJson(doc, res);
        server.send(200, "application/json", res);
    });

    server.on("/api/toggle", HTTP_POST, []() {
        is_running = !is_running;
        if(is_running) {
             lv_label_set_text(label_btn_start, "STOP");
             lv_obj_set_style_bg_color(btn_start, lv_color_hex(0xCC0000), 0);
             ledcWrite(PWM_CH_FAN, 255);
        } else {
             lv_label_set_text(label_btn_start, "START");
             lv_obj_set_style_bg_color(btn_start, lv_color_hex(0x00AA00), 0);
             ledcWrite(PWM_CH_HEATER, 0); ledcWrite(PWM_CH_FAN, 0);
        }
        server.send(200, "text/plain", "OK");
    });

    server.on("/api/set_filament", HTTP_POST, []() {
        if (server.hasArg("val")) {
            current_filament = server.arg("val");
            if (current_filament == "PLA")  { remaining_seconds = 5 * 3600; pid_Setpoint = 50; }
            if (current_filament == "PETG") { remaining_seconds = 6 * 3600; pid_Setpoint = 65; }
            if (current_filament == "ABS")  { remaining_seconds = 6 * 3600; pid_Setpoint = 80; }
            if (current_filament == "TPU")  { remaining_seconds = 8 * 3600; pid_Setpoint = 55; }
            if (current_filament == "Custom") { remaining_seconds = custom_time; pid_Setpoint = custom_temp; }
            
            update_timer_label();
            lv_label_set_text_fmt(label_target, "Target: %d C", (int)pid_Setpoint);
        }
        server.send(200, "text/plain", "OK");
    });

    server.on("/api/adj_time", HTTP_POST, []() {
        if(server.hasArg("val")) {
            long adj = server.arg("val").toInt();
            remaining_seconds += adj;
            if(remaining_seconds < 0) remaining_seconds = 0;
            update_timer_label();
        }
        server.send(200, "text/plain", "OK");
    });

    server.on("/api/adj_target", HTTP_POST, []() {
        if(server.hasArg("val")) {
            float adj = server.arg("val").toFloat();
            pid_Setpoint += adj;
            if(pid_Setpoint < 20.0) pid_Setpoint = 20.0;
            if(pid_Setpoint > 90.0) pid_Setpoint = 90.0;
            lv_label_set_text_fmt(label_target, "Target: %d C", (int)pid_Setpoint);
        }
        server.send(200, "text/plain", "OK");
    });

    server.on("/api/config", HTTP_GET, []() {
        JsonDocument doc;
        doc["c_temp"] = custom_temp;
        doc["c_time"] = custom_time / 3600; // Send hours to WebUI
        
        doc["hostname"] = hostname;
        doc["kp"] = Kp; doc["ki"] = Ki; doc["kd"] = Kd;
        doc["dhcp"] = use_dhcp;

        prefs.begin("drybox", true);
        if(use_dhcp) {
            doc["ip"] = WiFi.localIP().toString();
            doc["gw"] = WiFi.gatewayIP().toString();
            doc["nm"] = WiFi.subnetMask().toString();
        } else {
            doc["ip"] = prefs.getString("ip", "192.168.1.100");
            doc["gw"] = prefs.getString("gw", "192.168.1.1");
            doc["nm"] = prefs.getString("nm", "255.255.255.0");
        }
        
        doc["mqtt"] = prefs.getString("mqtt", "");
        doc["m_port"] = prefs.getString("m_port", "1883");
        doc["m_user"] = prefs.getString("m_user", "");
        doc["m_pass"] = prefs.getString("m_pass", "");
        doc["scr_off"] = auto_screen_off;
        prefs.end();
        
        String res; serializeJson(doc, res);
        server.send(200, "application/json", res);
    });

    server.on("/api/save_config", HTTP_POST, []() {
        if (server.hasArg("plain")) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, server.arg("plain"));
            
            if (!error) {
                prefs.begin("drybox", false);
                
                // Save Custom Profile
                custom_temp = doc["c_temp"] | 50.0;
                custom_time = (doc["c_time"] | 5) * 3600; // Convert hours to seconds
                prefs.putFloat("c_temp", custom_temp);
                prefs.putLong("c_time", custom_time);

                // Save Hostname
                String new_host = doc["hostname"] | "IkeDryBox";
                prefs.putString("hostname", new_host);
                hostname = new_host; 
                
                // Save PID
                Kp = doc["kp"]; Ki = doc["ki"]; Kd = doc["kd"];
                boxPID.SetTunings(Kp, Ki, Kd);
                prefs.putDouble("kp", Kp); prefs.putDouble("ki", Ki); prefs.putDouble("kd", Kd);
                
                // Save Rete
                use_dhcp = doc["dhcp"];
                prefs.putBool("dhcp", use_dhcp);
                
                if(!use_dhcp) {
                    prefs.putString("ip", doc["ip"] | "");
                    prefs.putString("gw", doc["gw"] | "");
                    prefs.putString("nm", doc["nm"] | "");
                }
                
                // Secure extract for MQTT
                prefs.putString("mqtt", doc["mqtt"] | "");
                prefs.putString("m_port", doc["m_port"] | "1883");
                prefs.putString("m_user", doc["m_user"] | "");
                prefs.putString("m_pass", doc["m_pass"] | "");
                
                // Save Screen Off
                auto_screen_off = doc["scr_off"] | false;
                prefs.putBool("scr_off", auto_screen_off);
                
                prefs.end();
            }
        }
        server.send(200, "text/plain", "OK");
        delay(500); 
        ESP.restart(); 
    });

    server.on("/api/info", HTTP_GET, []() {
        JsonDocument doc;
        
        doc["fw_version"] = String(FIRMWARE_VERSION) + " (IkeDryBox)"; // Centralized firmware version fron globals.h
        doc["sdk"] = ESP.getSdkVersion();
        doc["chip"] = ESP.getChipModel();
        
        uint32_t up = millis() / 1000;
        int d = up / 86400; up %= 86400;
        int h = up / 3600; up %= 3600;
        int m = up / 60; int s = up % 60;
        char up_buf[32];
        if(d > 0) sprintf(up_buf, "%dT%02d:%02d:%02d", d, h, m, s);
        else sprintf(up_buf, "%02d:%02d:%02d", h, m, s);
        doc["uptime"] = up_buf;

        doc["ssid"] = WiFi.SSID();
        doc["rssi"] = String(WiFi.RSSI()) + " dBm";
        doc["mac"] = WiFi.macAddress();
        doc["ip"] = WiFi.localIP().toString();
        doc["gw"] = WiFi.gatewayIP().toString();
        doc["nm"] = WiFi.subnetMask().toString();
        
        doc["free_heap"] = String(ESP.getFreeHeap() / 1024) + " KB";
        doc["flash_size"] = String(ESP.getFlashChipSize() / 1024) + " KB";
        doc["sketch_size"] = String(ESP.getSketchSize() / 1024) + " KB";
        doc["free_sketch"] = String(ESP.getFreeSketchSpace() / 1024) + " KB";

        String res; serializeJson(doc, res);
        server.send(200, "application/json", res);
    });

    server.on("/api/ota", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        
        delay(1000);             
        WiFi.disconnect(true);   
        WiFi.mode(WIFI_OFF);     
        delay(500);              
        ESP.restart();           
    }, []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("Update started OTA: %s\n", upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { 
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) { 
                Serial.printf("Update OTA Completed! Dimension: %u\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
        }
    });

    server.begin();
}

void handle_web_server() { server.handleClient(); }