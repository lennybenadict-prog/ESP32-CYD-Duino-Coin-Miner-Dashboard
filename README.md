# ⛏️ ESP32 CYD Duino-Coin Miner & Dashboard
![IMG_20260224_092323](https://github.com/user-attachments/assets/5cf61f58-acb7-42d4-b042-07ff172417c9)


Pushing the ESP32 to its limits. This is a custom, memory-optimized Duino-Coin (DUCO) miner built for the "Cheap Yellow Display" (ESP32-2432S028R). By stripping out Arduino `String` objects and using FreeRTOS to pin a C++ hashing loop to Core 0, this rig hits a massive **27.1 kH/s** while running a buttery-smooth live dashboard and background OTA web server on Core 1.

## ✨ Features
* **Extreme Optimization:** Rewritten SHA-1 hashing loop uses raw C++ byte arrays instead of memory-fragmenting `Strings`, achieving 27+ kH/s.
* **Dual-Core Processing:** Core 0 is locked at 100% for mining, while Core 1 handles Wi-Fi, API fetching, and display rendering without stuttering.
* **Live ECG Hashrate Graph:** A real-time scrolling graph charting your mining speed.
* **Over-The-Air (OTA) Updates:** Includes a hidden background Web Server. Compile `.bin` files and flash the board wirelessly via a web browser.
* **Touch-to-Sleep:** Tap the touchscreen to turn off the bright LCD backlight at night while the miner continues running silently in the background.
* **Live API Data:** Automatically pulls your live DUCO balance, USD equivalent, and estimates daily earnings.

## 🛠️ Hardware Requirements
* **ESP32-2432S028R** (Cheap Yellow Display)
* 5V / 2A USB Wall Charger (Do not run off a PC USB port, it will brownout!)

## 📦 Software Dependencies
You will need the Arduino IDE with the **ESP32 Core** installed (Version 3.x.x is fully supported). Install the following via the Arduino Library Manager:
* `TFT_eSPI` by Bodmer (Graphics)
* `ArduinoJson` by Benoit Blanchon (API parsing)

## ⚠️ CRITICAL SETUP: TFT_eSPI Configuration
Before compiling, you **MUST** configure the `TFT_eSPI` library to work with the CYD pins, otherwise the screen will be blank or throw a compile error.

1. Navigate to your Arduino libraries folder (usually `Documents/Arduino/libraries/TFT_eSPI`).
2. Open the `User_Setup.h` file.
3. Delete everything inside and replace it with this specific CYD configuration:

```cpp
#define USER_SETUP_INFO "User_Setup"
#define ILI9341_DRIVER
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  -1   
#define TFT_BL   21   
#define TOUCH_CS 33   
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define SPI_FREQUENCY  55000000
#define SPI_TOUCH_FREQUENCY  2500000
```

## 🚀 Installation & Configuration
1. Clone this repository and open the .ino file in the Arduino IDE.
2. At the top of the code, update your user credentials:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
String ducoUsername = "YOUR_DUCO_USERNAME"; 
String minerKey = "YOUR_MINER_KEY"; // Leave blank if you don't use a key
```
3. Connect your CYD to your PC via USB and hit Upload.
4. Once it boots, it will connect to Wi-Fi, calibrate with the Kolka load balancer, and automatically begin mining!

## 📲 OTA Wireless Updates
1. Export your compiled .bin file from the Arduino IDE.
2. Find your ESP32's IP address on your local network.
3. Open a web browser and go to http://[YOUR_ESP32_IP]/update.
4. Log in (Default User: admin, Pass: admin).
5. Upload the .bin file. The board will flash and reboot automatically.

## 🙏 Credits & Acknowledgments
None of this would have been possible without the open-source community:

* The Duino-Coin Team (Revox & Co.): For creating an accessible crypto network that gives microcontrollers a genuine purpose.
* Bodmer (TFT_eSPI): For the legendary graphics library that makes the ECG graph incredibly fast and memory-efficient.
* Benoit Blanchon (ArduinoJson): For the bulletproof JSON parser.
* Espressif Systems: For the mbedtls hardware acceleration libraries.
* Gemini: My AI pair-programmer that helped debug Watchdog crashes, untangle V3 library changes, and optimize the C++ memory management.
