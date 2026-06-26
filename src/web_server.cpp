// File: src/web_server.cpp
#include "globals.h"
#include "web_server.h"
#include "ui_main.h"
#include "mqtt_manager.h"
#include <WebServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Update.h>
#include <nvs_flash.h>

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
        
        /* Tooltip container */
        .tooltip { position: relative; display: inline-block; cursor: pointer; margin-left: 5px; color: #00BFFF; font-weight: bold; }
        /* Tooltip text */
        .tooltip .tooltiptext { visibility: hidden; width: 260px; background-color: #222; color: #fff; text-align: left; border-radius: 6px; padding: 12px; position: absolute; z-index: 10; bottom: 125%; left: 50%; margin-left: -130px; opacity: 0; transition: opacity 0.3s; font-size: 0.8em; line-height: 1.4; border: 1px solid #555; box-shadow: 0 4px 12px rgba(0,0,0,0.6); font-weight: normal; pointer-events: none; }
        /* Show the tooltip text when hovering */
        .tooltip:hover .tooltiptext { visibility: visible; opacity: 1; }
    </style>
    <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.7/dist/chart.umd.min.js"></script>
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
            <button class="btn-main" style="background:#444;" onclick="toggleCharts()">CHARTS</button>
        </div>
        
        <div id="config" class="panel">
            <h3 style="color:#00FF00; margin-top: 5px;">Custom Profile</h3>
            <div class="input-group"><label>Target Temp (&deg;C):</label><input type="number" id="c_temp" min="20" max="90"></div>
            <div class="input-group">
                <label>Timer (H:M):</label>
                <div style="display:flex; gap:5px;">
                    <input type="number" id="c_time_h" min="0" max="24" style="width:70px;" placeholder="Hours">
                    <input type="number" id="c_time_m" min="0" max="59" style="width:70px;" placeholder="Mins">
                </div>
            </div>
            <hr style="border: 0; border-top: 1px solid #333; margin: 25px 0;">

            <h3 style="color:#FFAA00; margin-bottom: 5px;">PID Tuning</h3>
            <div style="font-size: 0.85em; color: #aaa; margin-bottom: 15px; line-height: 1.4; background: #1a1a1a; padding: 10px; border-radius: 5px;">
                <strong>Kp (Proportional):</strong> Pushes the heater. Higher values heat up faster but can cause overshoot.<br>
                <span style="display:block; margin-top:4px;"><strong>Ki (Integral):</strong> Corrects long-term errors to hit the exact target. Too high causes temperature oscillation.</span>
                <span style="display:block; margin-top:4px;"><strong>Kd (Derivative):</strong> Acts as a brake as it approaches the target to prevent overshooting. Increases stability.</span>
            </div>
            
            <div class="input-group"><label>Kp:</label><input type="number" id="kp"></div>
            <div class="input-group"><label>Ki:</label><input type="number" step="0.01" id="ki"></div>
            <div class="input-group"><label>Kd:</label><input type="number" step="0.1" id="kd"></div>
            
            <h3 style="color:#00BFFF">System</h3>
            <div class="input-group"><label>Screen Timeout (min, 0=off):</label><input type="number" id="scr_to" min="0" step="1"></div>
            <div class="input-group"><label>Red LED:</label><input type="range" id="led_r" min="0" max="255" value="10" oninput="document.getElementById('led_r_val').innerText=this.value"><span id="led_r_val" style="min-width:30px;text-align:center;">10</span></div>
            <div class="input-group"><label>Green LED:</label><input type="range" id="led_g" min="0" max="255" value="10" oninput="document.getElementById('led_g_val').innerText=this.value"><span id="led_g_val" style="min-width:30px;text-align:center;">10</span></div>
            <div class="input-group">
                <label>Display Board Type:</label>
                <select id="is_st7789" style="background:#444; color:white; padding:5px; border-radius:4px; width:150px;">
                    <option value="false">ILI9341 (Original)</option>
                    <option value="true">ST7789 (New)</option>
                </select>
            </div>

            <hr style="border: 0; border-top: 1px solid #333; margin: 20px 0;">

            <h3 style="color:#00FF00">Network</h3>
            <div class="input-group"><label>Hostname:</label><input type="text" id="hostname"></div>
            <div class="input-group"><label>Use DHCP:</label><input type="checkbox" id="dhcp" onchange="toggleDhcp(this.checked)" style="width:20px; height:20px;"></div>
            <div class="input-group"><label>Static IP:</label><input type="text" id="ip"></div>
            <div class="input-group"><label>Gateway:</label><input type="text" id="gw"></div>
            <div class="input-group"><label>Netmask:</label><input type="text" id="nm"></div>
            <div class="input-group">
                <label>
                    Share Anonymous Telemetry 
                    <span class="tooltip">ⓘ<span class="tooltiptext">Collected data is completely anonymous (duration, temp, filament type, and firmware version) and does not contain any personal or network details (such as IP or MAC). Sharing this data is free of charge, but it helps me greatly to improve the project and its development!</span></span>:
                </label>
                <input type="checkbox" id="telemetry" style="width:20px; height:20px;">
            </div>

            <hr style="border: 0; border-top: 1px solid #333; margin: 20px 0;">

            <h3 style="color:#FFAA00">MQTT</h3>
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
            <hr style="border: 0; border-top: 1px solid #333; margin: 20px 0;">
            <h3 style="color:#FFAA00;">Backup</h3>
            <div style="display:flex; gap:10px;">
                <button class="btn-main" style="background:#0088FF; padding:10px;" onclick="exportSettings()">EXPORT SETTINGS</button>
            </div>
            <div style="display:flex; gap:10px; margin-top:10px; align-items:center;">
                <input type="file" id="import_file" accept=".json" style="flex:1;">
                <button class="btn-main" style="background:#FFAA00; padding:10px; width:auto;" onclick="importSettings()">IMPORT</button>
            </div>
            <hr style="border: 0; border-top: 1px solid #333; margin: 20px 0;">
            <h3 style="color:#FF3333;">Factory Reset</h3>
            <div style="background: #1a1a1a; padding: 10px; border-radius: 5px; text-align:center;">
                <div style="color:#FF6666; font-size:0.85em; margin-bottom:10px;">Resets all settings, MQTT, PID, network &amp; WiFi credentials</div>
                <button class="btn-main" style="background:#CC0000; padding:10px;" onclick="factoryReset()">FACTORY RESET</button>
            </div>
        </div>

        <div id="chart_panel" class="panel">
            <h3 style="color:#FFAA00; margin-top: 0;">Charts</h3>
            <div class="input-group"><label>Data points:</label><div style="display:flex; gap:5px;"><input type="number" id="chrt_pts" min="60" max="3600" step="1"><button onclick="saveChartPoints()" style="background:#0088FF; padding:5px 10px; border-radius:4px; font-size:0.9em;">SAVE</button></div></div>
            <hr style="border: 0; border-top: 1px solid #333; margin: 15px 0;">
            <div style="position:relative; height:200px;"><canvas id="chart_temp"></canvas></div>
            <div style="position:relative; height:200px; margin-top:15px;"><canvas id="chart_hum"></canvas></div>
            <div style="position:relative; height:200px; margin-top:15px;"><canvas id="chart_power"></canvas></div>
        </div>
    </div>

    <script>
        let chartMaxPoints = 900;
        let timeLabels = [], tempData = [], targetData = [];
        let humData = [], heaterData = [], fanData = [];
        let chartsInitialized = false;
        let chartTemp = null, chartHum = null, chartPower = null;

        function pushChartData(d) {
            timeLabels.push(new Date().toLocaleTimeString());
            tempData.push(d.temp);
            targetData.push(d.target);
            humData.push(d.hum);
            heaterData.push(d.heater);
            fanData.push(d.fan);
            if (timeLabels.length > chartMaxPoints) {
                timeLabels.shift(); tempData.shift(); targetData.shift();
                humData.shift(); heaterData.shift(); fanData.shift();
            }
            if (chartTemp) chartTemp.update('none');
            if (chartHum) chartHum.update('none');
            if (chartPower) chartPower.update('none');
        }

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
                    if (d.time_str === "0:00:00") {
                        stat.innerText = "DONE!"; stat.style.color = "#00FF00";
                    } else {
                        stat.innerText = "READY"; stat.style.color = "#aaa";
                    }
                }
                pushChartData(d);
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
                    document.getElementById('c_time_h').value = c.c_time_h;
                    document.getElementById('c_time_m').value = c.c_time_m;
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
                    document.getElementById('scr_to').value = c.scr_to; // Update input field with minutes
                    document.getElementById('led_r').value = c.led_r;
                    document.getElementById('led_g').value = c.led_g;
                    document.getElementById('led_r_val').innerText = c.led_r;
                    document.getElementById('led_g_val').innerText = c.led_g;
                    document.getElementById('is_st7789').value = c.is_st7789 ? "true" : "false";
                    document.getElementById('telemetry').checked = c.telemetry;
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
                    addRow("DNS Server", d.dns);
                    html += `<tr><td colspan="2"><hr style="border:0; border-top:1px solid #333; margin:5px 0;"></td></tr>`;
                    addRow("Flash Size", d.flash_size);
                    addRow("Program Size", d.sketch_size);
                    addRow("Free Prg Space", d.free_sketch);
                    addRow("Free Memory", d.free_heap);

                    document.getElementById('info_table').innerHTML = html;
                });
            }
        }

        function toggleCharts() {
            let p = document.getElementById('chart_panel');
            let conf = document.getElementById('config');
            let info = document.getElementById('info_panel');
            conf.style.display = 'none';
            info.style.display = 'none';
            p.style.display = (p.style.display == 'block') ? 'none' : 'block';
            if(p.style.display == 'block') {
                fetch('/api/config').then(r => r.json()).then(c => {
                    chartMaxPoints = c.chrt_pts || 900;
                    document.getElementById('chrt_pts').value = chartMaxPoints;
                });
                if (!chartsInitialized) {
                    initCharts();
                    chartsInitialized = true;
                }
            }
        }

        function initCharts() {
            const opts = {
                responsive: true, maintainAspectRatio: false,
                animation: false,
                plugins: { legend: { labels: { color: '#aaa', boxWidth: 12 } } },
                scales: {
                    x: { ticks: { color: '#888', maxTicksLimit: 6 }, grid: { color: '#333' } },
                    y: { ticks: { color: '#888' }, grid: { color: '#333' } }
                }
            };
            chartTemp = new Chart(document.getElementById('chart_temp'), {
                type: 'line',
                data: {
                    labels: timeLabels,
                    datasets: [
                        { label: 'Temp °C', data: tempData, borderColor: '#FF3333', backgroundColor: 'transparent', tension: 0.3, pointRadius: 0 },
                        { label: 'Target °C', data: targetData, borderColor: '#FFAA00', backgroundColor: 'transparent', tension: 0.3, pointRadius: 0, borderDash: [4,4] }
                    ]
                },
                options: opts
            });
            chartHum = new Chart(document.getElementById('chart_hum'), {
                type: 'line',
                data: {
                    labels: timeLabels,
                    datasets: [
                        { label: 'Humidity %', data: humData, borderColor: '#00BFFF', backgroundColor: 'transparent', tension: 0.3, pointRadius: 0 }
                    ]
                },
                options: opts
            });
            chartPower = new Chart(document.getElementById('chart_power'), {
                type: 'line',
                data: {
                    labels: timeLabels,
                    datasets: [
                        { label: 'Heater %', data: heaterData, borderColor: '#FF6600', backgroundColor: 'transparent', tension: 0.3, pointRadius: 0 },
                        { label: 'Fan %', data: fanData, borderColor: '#00FF00', backgroundColor: 'transparent', tension: 0.3, pointRadius: 0 }
                    ]
                },
                options: opts
            });
        }

        function saveChartPoints() {
            let val = parseInt(document.getElementById('chrt_pts').value) || 900;
            fetch('/api/save_chart_pts?val=' + val, {method: 'POST'}).then(() => {
                document.getElementById('chrt_pts').style.borderColor = '#00FF00';
                setTimeout(() => document.getElementById('chrt_pts').style.borderColor = '', 2000);
            });
        }

        function saveConfig() {
            let data = {
                scr_to: parseInt(document.getElementById('scr_to').value) || 0, // Parse minutes from input
                c_temp: parseFloat(document.getElementById('c_temp').value),
                c_time_h: parseInt(document.getElementById('c_time_h').value) || 0,
                c_time_m: parseInt(document.getElementById('c_time_m').value) || 0,
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
                m_pass: document.getElementById('m_pass').value,
                led_r: parseInt(document.getElementById('led_r').value),
                led_g: parseInt(document.getElementById('led_g').value),
                is_st7789: document.getElementById('is_st7789').value === "true",
                telemetry: document.getElementById('telemetry').checked
            };
            fetch('/api/save_config', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify(data)
            }).then(() => alert("Settings Saved!\n(Device will reboot if Display Board Type was changed)"));
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

        function exportSettings() {
            fetch('/api/config').then(r => r.json()).then(d => {
                let blob = new Blob([JSON.stringify(d, null, 2)], {type: 'application/json'});
                let a = document.createElement('a');
                a.href = URL.createObjectURL(blob);
                a.download = 'IkeDryBox-settings.json';
                a.click();
            });
        }

        function importSettings() {
            let fileInput = document.getElementById('import_file');
            let file = fileInput.files[0];
            if (!file) { alert("Please select a .json file first!"); return; }
            let reader = new FileReader();
            reader.onload = function(e) {
                fetch('/api/save_config', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: e.target.result
                }).then(r => {
                    if(r.ok) alert("Settings imported successfully!");
                    else alert("Import failed!");
                });
            };
            reader.readAsText(file);
        }

        function factoryReset() {
            if (!confirm("WARNING: This will delete ALL settings, MQTT config, network, PID,\nLED values and WiFi credentials.\n\nThe device will restart as if freshly flashed.\n\nContinue?")) return;
            if (!confirm("FINAL CONFIRMATION:\n\nThis action CANNOT be undone.\nAll data in NVS memory will be erased.\n\nProceed?")) return;
            fetch('/api/factory_reset', {method: 'POST'}).then(r => {
                if(r.ok) alert("Factory reset complete! Device will reboot...");
            });
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
        doc["hum"] = current_humidity; // <-- Read global variable for fix NaN
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
        
        doc["led_r"] = led_r_intensity;
        doc["led_g"] = led_g_intensity;
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
             // AUTO-RESET TIMER AT ZERO
             if (remaining_seconds <= 0) {
                 current_filament.trim(); // String cleaning
                 
                 if (current_filament == "PLA")  remaining_seconds = 5 * 3600;
                 else if (current_filament == "PETG") remaining_seconds = 6 * 3600;
                 else if (current_filament == "ABS")  remaining_seconds = 6 * 3600;
                 else if (current_filament == "TPU")  remaining_seconds = 8 * 3600;
                 else if (current_filament == "Custom") remaining_seconds = custom_time;
                 else remaining_seconds = 5 * 3600; // Fallback
                 
                 // Second safety net
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
             
             ledcWrite(PWM_CH_HEATER, 0); ledcWrite(PWM_CH_FAN, 0);
        }
        server.send(200, "text/plain", "OK");
    });

    server.on("/api/set_filament", HTTP_POST, []() {
        if (server.hasArg("val")) {
            current_filament = server.arg("val");
            if (current_filament == "PLA")  { remaining_seconds = TIME_PLA; pid_Setpoint = TEMP_PLA; }
            if (current_filament == "PETG") { remaining_seconds = TIME_PETG; pid_Setpoint = TEMP_PETG; }
            if (current_filament == "ABS")  { remaining_seconds = TIME_ABS; pid_Setpoint = TEMP_ABS; }
            if (current_filament == "TPU")  { remaining_seconds = TIME_TPU; pid_Setpoint = TEMP_TPU; }
            // if (current_filament == "Custom") { remaining_seconds = custom_time; pid_Setpoint = custom_temp; }
            if (current_filament == "Custom") { remaining_seconds = custom_time; pid_Setpoint = custom_temp; } // <-- 10 SECONDs for debug
            
            update_timer_label();
            lv_label_set_text_fmt(label_target, "Target: %d C", (int)pid_Setpoint);
            
            // Sync the physical display dropdown
            int fil_idx = 0;
            if (current_filament == "PLA") fil_idx = 0;
            else if (current_filament == "PETG") fil_idx = 1;
            else if (current_filament == "ABS") fil_idx = 2;
            else if (current_filament == "TPU") fil_idx = 3;
            else if (current_filament == "Custom") fil_idx = 4;
            if (dd_filament != NULL) lv_dropdown_set_selected(dd_filament, fil_idx);
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
        doc["c_time_h"] = custom_time / 3600; // Send hours to WebUI
        doc["c_time_m"] = (custom_time % 3600) / 60; // Send minitues to WebUI
        
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
        doc["scr_to"] = screen_timeout_mins; // Pass the timeout value in minutes to WebUI
        doc["led_r"] = prefs.getInt("led_r", 10);
        doc["led_g"] = prefs.getInt("led_g", 10);
        doc["chrt_pts"] = prefs.getInt("chrt_pts", 900);
        doc["is_st7789"] = is_st7789;
        doc["telemetry"] = opt_in_telemetry;
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
                long c_h = doc["c_time_h"] | 5;
                long c_m = doc["c_time_m"] | 0;
                custom_time = (c_h * 3600) + (c_m * 60); // Convert hours to seconds
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
                // --- NEW SCREEN TIMEOUT LOGIC ---
                // If not provided, fallback to the default set in globals.h
                screen_timeout_mins = doc["scr_to"] | DEFAULT_SCREEN_TIMEOUT;
                prefs.putInt("scr_to", screen_timeout_mins);

                // If minutes are greater than 0, automatically enable auto-screen-off
                auto_screen_off = (screen_timeout_mins > 0);
                prefs.putBool("scr_off", auto_screen_off);
                // ------------------------------------
                
                // Save LED intensity (only if provided, prevent overriding saved values)
                if (doc["led_r"].is<int>()) {
                    led_r_intensity = constrain(doc["led_r"].as<int>(), 0, 255);
                    prefs.putInt("led_r", led_r_intensity);
                }
                if (doc["led_g"].is<int>()) {
                    led_g_intensity = constrain(doc["led_g"].as<int>(), 0, 255);
                    prefs.putInt("led_g", led_g_intensity);
                }
                
                // Save chart data points
                if (doc["chrt_pts"].is<int>()) {
                    chart_max_points = constrain(doc["chrt_pts"].as<int>(), 60, 3600);
                    prefs.putInt("chrt_pts", chart_max_points);
                }

                // Save hardware version selection
                bool should_reboot = false;
                if (doc["is_st7789"].is<bool>()) {
                    bool new_st = doc["is_st7789"].as<bool>();
                    if (new_st != is_st7789) {
                        is_st7789 = new_st;
                        prefs.putBool("is_st7789", is_st7789);
                        should_reboot = true;
                    }
                }
                
                if (doc["telemetry"].is<bool>()) {
                    opt_in_telemetry = doc["telemetry"].as<bool>();
                    prefs.putBool("telemetry", opt_in_telemetry);
                }
                
                prefs.end();

                // Apply hostname live (takes effect for DHCP on next reconnect)
                WiFi.setHostname(hostname.c_str());

                // Force MQTT reconnect so new broker/credentials take effect immediately
                mqtt_force_reconnect();

                if (should_reboot) {
                    server.send(200, "text/plain", "OK");
                    delay(1000);
                    ESP.restart();
                    return;
                }
            }
        }
        server.send(200, "text/plain", "OK");
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
        doc["dns"] = WiFi.dnsIP(0).toString();
        
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

    server.on("/api/save_chart_pts", HTTP_POST, []() {
        if (server.hasArg("val")) {
            int val = constrain(server.arg("val").toInt(), 60, 3600);
            chart_max_points = val;
            prefs.begin("drybox", false);
            prefs.putInt("chrt_pts", val);
            prefs.end();
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "Missing val");
        }
    });

    server.on("/api/factory_reset", HTTP_POST, []() {
        server.send(200, "text/plain", "OK");
        delay(100);

        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);

        nvs_flash_erase();
        nvs_flash_init();

        ESP.restart();
    });

    server.begin();
}

void handle_web_server() { server.handleClient(); }