# 📦 IkeDryBox - Smart 3D Filament Dryer

IkeDryBox is an advanced, ESP32-based DIY smart filament dryer for 3D printing. Built with precision and efficiency in mind, it features a responsive LVGL touch interface, highly accurate PID temperature control, and seamless integration with Home Assistant via MQTT.

## ✨ Features

* **🌡️ Precision PID Control:** Custom-tuned Proportional-Integral-Derivative (PID) algorithm to maintain the exact target temperature without fluctuations.
* **📱 Touchscreen UI:** Beautiful and responsive graphical interface built with LVGL on a 2.4" TFT Display (CYD - Cheap Yellow Display).
* **🖥️ WebUI Configuration:** Built-in web server to tune PID parameters, configure network settings, and set MQTT credentials on the fly without reflashing.
* **🏠 Home Assistant Integration:** Full MQTT support with Auto-Discovery. Monitor temperature, humidity, remaining time, and control the dryer directly from your smart home dashboard.
* **🤫 Silent PWM Fan Control:** Hardware-specific low-frequency PWM tuning (100Hz) to eliminate coil whine and keep the fan whisper-quiet.
* **🛡️ Smart Screen Saver:** Auto-dimming "Touch Shield" that turns the screen completely black after 10 minutes of inactivity, protecting the display and preventing phantom touches.
* **💡 RGB Status LED:** Visual feedback for heating status (pulsing red) and standby mode (dimmed green) using a common-anode RGB LED.

## 🛠️ Hardware Requirements

* **Microcontroller:** ESP32-2432S028R (2.4" Cheap Yellow Display with resistive touch) or similar ESP32 board with TFT.
* **Sensor:** SHT31 (High precision Temperature & Humidity sensor via I2C).
* **Heater:** PTC heating element controlled via a solid-state relay or high-power MOSFET.
* **Fan:** 12V/24V DC Fan controlled via MOSFET.
* **LED:** Common-Anode RGB LED.

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

![IkeDryBox Wiring Diagram](IkeDryBox_wiring_diagram.jpg)
*(Upload the wiring diagram image to your repository and name it `IkeDryBox_wiring_diagram.jpg` to display it here).*

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

## 🧠 PID Tuning Tips
The default PID values (Kp: 60.0, Ki: 0.6, Kd: 8.0) are tuned for a standard enclosed box with a fast-acting PTC heater. If you notice temperature instability:

1. Overshoots the target: Increase Kd or decrease Kp.

2. Stops below the target: Increase Ki to accumulate the steady-state error.

3. Oscillates continuously: Decrease Ki and Kp.


📄 License
This project is open-source and available under the MIT License.
