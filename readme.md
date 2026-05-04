# 📦 IkeDryBox - Smart 3D Filament Dryer

IkeDryBox is an advanced, ESP32-based DIY smart filament dryer for 3D printing. Built with precision and efficiency in mind, it features a responsive LVGL touch interface, highly accurate PID temperature control, and seamless integration with Home Assistant via MQTT.
![IkeDryBox dashboard](https://github.com/byte4geek/IkeDryBox/blob/main/images/main_ui.jpg)

## ✨ Features

* **🌡️ Precision PID Control:** Custom-tuned Proportional-Integral-Derivative (PID) algorithm to maintain the exact target temperature without fluctuations.
* **📱 Touchscreen UI:** Beautiful and responsive graphical interface built with LVGL on a 2.4" TFT Display (CYD - Cheap Yellow Display fron TZT electronics).
* **🖥️ WebUI Configuration:** Built-in web server to tune PID parameters, configure network settings, and set MQTT credentials on the fly without reflashing.
* **🏠 Home Assistant Integration:** Full MQTT support with Auto-Discovery. Monitor temperature, humidity, remaining time, and control the dryer directly from your smart home dashboard.
* **🤫 Silent PWM Fan Control:** Hardware-specific low-frequency PWM tuning (100Hz) to eliminate coil whine and keep the fan whisper-quiet.
* **🛡️ Smart Screen Saver:** Auto-dimming "Touch Shield" that turns the screen completely black after 10 minutes of inactivity, protecting the display and preventing phantom touches.
* **💡 RGB Status LED:** Visual feedback for heating status (pulsing red) and standby mode (dimmed green) using a common-anode of the builtin RGB LED.

## 🛠️ Hardware Requirements

* **Microcontroller:** ESP32 LVGL with builtin LCD color display (2.4" Cheap Yellow Display with resistive touch and builtin RGB led mounted on front of the board) or similar ESP32 board with TFT. [LCD Board](https://it.aliexpress.com/item/1005008212152877.html) more description and documentations regarding this board are available here [Board Documentations](https://www.tztstore.com/goods/show-7983.html)
* **Sensor:** SHT31 (High precision Temperature & Humidity sensor via I2C). [SHT31 sensor](https://s.click.aliexpress.com/e/_c39bmlmr)
* **Heater:** PTC heating with fan element controlled via high-power MOSFET. [PCT Heater + Fan](https://s.click.aliexpress.com/e/_c3vN8PiX)
* **MOSFET** AOD4184 - 40V 50A. [Mosfet board](https://s.click.aliexpress.com/e/_c3yDwbWf)
* **Temperature Switch Thermostat** 85°C 10A Ceramic Hole-NC [Thermal Protection switch](https://s.click.aliexpress.com/e/_c3eNqQKF)
* **Power supply 12V 10A 120W** [Power supply](https://s.click.aliexpress.com/e/_c3uwulYb)

## 📌 Pinout Configuration

Key hardware connections based on the wiring diagram:

| Component | ESP32 Pin | Note |
| :--- | :--- | :--- |
| **SHT31 SDA** | `21` | I2C Data |
| **SHT31 SCL** | `22` | I2C Clock |
| **RGB LED (Red)** | `4` | PWM Controlled (Inverted Logic) |
| **RGB LED (Green)**| `17` | PWM Controlled (Inverted Logic) |
| **Heater MOSFET** | `5` | 1000Hz PWM for stable heating |
| **Fan MOSFET** | `23` | 100Hz PWM for silent operation |

## 🔌 Wiring Diagram

![IkeDryBox Wiring Diagram](https://github.com/byte4geek/IkeDryBox/blob/main/images/IkeDryBox_wiring_diagram.png)

## 💻 Software Dependencies

This project is built using **PlatformIO**. Make sure to install the following libraries:

* `LovyanGFX` (for TFT and Touch drivers)
* `lvgl` (UI Library, v8.x recommended)
* `Adafruit SHT31 Library`
* `PID_v1`
* `WiFiManager`
* `PubSubClient` (for MQTT)

## 🚀 Installation & Setup

1. **Clone the repository:**
   ```bash
   git clone https://github.com/byte4geek/IkeDryBox.git
   ```

2. Open the project in VSCode with the PlatformIO extension.

3. Build and Upload the firmware and filesystem to your ESP32.

4. Initial Boot: The ESP32 will boot and create an Access Point named IkeDryBox_Setup.

5. Connect & Configure: Connect your phone/PC to this AP, wait for the captive portal, and enter your home WiFi credentials.

6. WebUI Setup: Once connected to your home network, find the device IP address on the screen. Open a browser and navigate to http://<DEVICE_IP> to tune your PID settings (Kp, Ki, Kd) and enter your MQTT broker details.
![WebUI](https://github.com/byte4geek/IkeDryBox/blob/main/images/WebUI.jpeg)

## 🧠 PID Tuning Tips
The default PID values (Kp: 60.0, Ki: 0.6, Kd: 8.0) are tuned for a standard enclosed box with a fast-acting PTC heater. If you notice temperature instability:

1. Overshoots the target: Increase Kd or decrease Kp.

2. Stops below the target: Increase Ki to accumulate the steady-state error.

3. Oscillates continuously: Decrease Ki and Kp.


## ⚠️ DANGER: HIGH VOLTAGE & SAFETY DISCLAIMER

**WARNING: This project involves working with mains electricity (110V/220V AC) which is EXTREMELY DANGEROUS and can cause severe injury, fire, or DEATH if handled incorrectly.**

By downloading, viewing, or attempting to replicate any part of this project, you fully acknowledge and agree to the following:

1. **You Are Solely Responsible:** You attempt this project entirely at your own risk. The author(s) of this project, the repository owner, and anyone associated with its creation accept **ZERO RESPONSIBILITY or LIABILITY** for any injuries, deaths, property damage, fires, or any other negative consequences resulting from the use or misuse of the information, code, or 3D models provided here.
2. **Required Knowledge:** This is NOT a beginner-level electronics project. Do not attempt to build this if you do not have a solid understanding of electrical safety, proper wiring techniques, grounding, and how to safely isolate high-voltage components from low-voltage microcontrollers.
3. **No Guarantees:** This project is provided "as-is" for educational and informational purposes only. There are no warranties, expressed or implied, regarding its safety, reliability, or suitability for any specific purpose. The code and hardware design may contain bugs or flaws.
4. **Thermal Hazards:** The PTC heater generates significant heat. Proper thermal insulation, airflow management, and the use of heat-resistant materials are mandatory to prevent melting or fire hazards. Never leave a DIY heating device running completely unattended without proper safety cutoffs (e.g., thermal fuses).

**IF YOU DO NOT KNOW WHAT YOU ARE DOING, STOP NOW. Do not touch mains voltage. Ask a qualified electrician for help.**

📄 License
This project is open-source and available under the MIT License.

# Donation
Buy me a coffee

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=VK4CSX9NVQAZU)
