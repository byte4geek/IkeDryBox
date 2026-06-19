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
* **Microcontroller:** ESP32 with ST7789 chip with builtin LCD color display (2.4" Cheap Yellow Display with resistive touch and builtin RGB led mounted on back of the board) or similar ESP32 board with TFT. [ST7789 LCD Board](https://s.click.aliexpress.com/e/_c3GVNG01) more description and documentations regarding this board are available here [Board Documentations](https://www.lcdwiki.com/2.8inch_ESP32-32E_Display)

    or

* **Microcontroller:** ESP32 LVGL with ILI9341 chip with builtin LCD color display (2.4" Cheap Yellow Display with resistive touch and builtin RGB led mounted on front of the board) or similar ESP32 board with TFT. [ILI9341 LCD Board](https://it.aliexpress.com/item/1005008212152877.html) more description and documentations regarding this board are available here [Board Documentations](https://www.tztstore.com/goods/show-7983.html)
* **Sensor:** SHT31 (High precision Temperature & Humidity sensor via I2C). [SHT31 sensor](https://s.click.aliexpress.com/e/_c39bmlmr)
* **Heater:** PTC heating with fan element controlled via high-power MOSFET. [PCT Heater + Fan](https://s.click.aliexpress.com/e/_c3vN8PiX)
* **MOSFET** AOD4184 - 40V 50A. [Mosfet board](https://s.click.aliexpress.com/e/_c3yDwbWf)
* **Temperature Switch Thermostat** 85°C 10A Ceramic Hole-NC [Thermal Protection switch](https://s.click.aliexpress.com/e/_c3eNqQKF)
* **Power supply 12V 10A 120W** [Power supply](https://s.click.aliexpress.com/e/_c3uwulYb)
* **Printables parts to assemble the project** [Printables parts](https://makerworld.com/it/models/2755143-ikedrybox-smart-active-drybox-ikea-samla-22l)
* **Box** For this DryBox I used an ikea samla 22lt container, but you can choose another thing to do it.

## 📌 Hardware Board & Pinout Comparison

IkeDryBox supports two distinct hardware configurations. You can select your board type before compiling by editing `include/globals.h`. Below is the pinout comparison between the original CYD (ILI9341) and the new 2.8" ESP32-32E (ST7789) board:

| Component | ILI9341 Board (Original CYD) | ST7789 Board (New ESP32-32E) | Connection Note |
| :--- | :---: | :---: | :--- |
| **Display Panel** | ILI9341 (2.4" TFT) | ST7789 (2.8" TFT) | SPI2 Bus |
| **Backlight Pin** | `27` | `21` | PWM Controlled |
| **Touch Controller** | XPT2046 (Shared SPI2) | XPT2046 (Dedicated SPI3) | SPI3 Pins: SCLK=25, MOSI=32, MISO=39, CS=33, INT=36 |
| **SHT31 SDA (I2C)** | `21` | `27` | High-precision Temp & Humidity sensor |
| **SHT31 SCL (I2C)** | `22` | `18` | High-precision Temp & Humidity sensor |
| **RGB LED (Red)** | `4` | `22` | Common Anode (Inverted logic, PWM controlled) |
| **RGB LED (Green)**| `17` | `16` | Common Anode (Inverted logic, PWM controlled) |
| **Heater MOSFET** | `5` | `19` | PWM (1000Hz) |
| **Fan MOSFET** | `23` | `23` | PWM (100Hz) |

---

## 🖥️ How to Select Your Hardware Board

To configure the firmware for your specific board type:

1. Open the project in VS Code with PlatformIO.
2. Open the file [include/globals.h](file:///c:/Antigravity/IkeDryBox/include/globals.h).
3. Scroll to the `--- HARDWARE BOARD VERSION SELECT ---` section:
   ```cpp
   // --- HARDWARE BOARD VERSION SELECT ---
   // Uncomment only ONE of the following lines depending on your board:
   #define HARDWARE_ILI9341  // Original board with ILI9341 screen driver
   // #define HARDWARE_ST7789   // New board with ST7789 screen driver
   ```
4. Comment/uncomment the appropriate line depending on whether you have the original 2.4" CYD board (`HARDWARE_ILI9341`) or the newer 2.8" board (`HARDWARE_ST7789`).
5. Save the file, then compile and upload the firmware.

### 🔌 Wiring Details for the ST7789 Board
Because of GPIO conflicts on the ST7789 board (where GPIO 21 is used for the backlight instead of I2C SDA, and GPIO 22 is used for the Red LED instead of I2C SCL), the I2C bus and other components are routed differently:
- **I2C Bus (SHT31 Sensor)**: Wire the SHT31 sensor to the 4-pin JST connector labeled `SPI Peripheral` (or I2C depending on the board version) using **GPIO 27 (SDA)** and **GPIO 18 (SCL)**.
- **Heater & Fan MOSFETs**: Wire the Heater MOSFET gate to **GPIO 19** and the Fan MOSFET gate to **GPIO 23** (also available on the JST peripheral connector).
- **RGB LEDs**: The board uses the onboard RGB LED pins **GPIO 22 (Red)** and **GPIO 16 (Green)**.

## 🔌 Wiring Diagrams

### ILI9341 Board (Original CYD)
![IkeDryBox Wiring Diagram ILI9341](https://github.com/byte4geek/IkeDryBox/blob/main/images/IkeDryBox_wiring_diagram.png)

### ST7789 Board (New ESP32-32E)
![IkeDryBox Wiring Diagram ST7789](https://github.com/byte4geek/IkeDryBox/blob/main/images/IkeDryBox_wiring_diagram_ST7789.png)

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
