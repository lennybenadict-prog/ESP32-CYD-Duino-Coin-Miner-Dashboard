# ⛏️ ESP32 CYD Duino-Coin Miner & Dashboard



A highly optimized, dual-core Duino-Coin (DUCO) miner built specifically for the ESP32-2432S028R "Cheap Yellow Display" (CYD). By utilizing raw C++ byte arrays, moving cryptography initialization out of the hashing loop, and leveraging FreeRTOS to split tasks across both cores, this miner achieves a massive **~27.1 kH/s** while maintaining a buttery-smooth, interactive UI.

## ✨ Features
* **Extreme Optimization:** Stripped of Arduino `String` objects in the hashing loop to prevent memory fragmentation, achieving 27+ kH/s.
* **Dual-Core Processing (FreeRTOS):** Core 0 handles the brutal SHA-1 hashing, while Core 1 handles the Wi-Fi, APIs, and display rendering.
* **Live ECG Hashrate Graph:** A real-time scrolling graph that charts your mining speed visually.
* **Over-The-Air (OTA) Updates:** Includes a hidden background Web Server. You can update the code wirelessly via a web browser (perfect for programming from an iPad or phone).
* **Touch-to-Sleep:** Tap the touchscreen to turn off the bright LCD backlight at night while the miner continues running silently in the background.
* **Live API Fetching:** Automatically pulls your live DUCO balance, USD equivalent, and calculates estimated daily earnings.

## 🛠️ Hardware Requirements
* **ESP32-2432S028R** (Cheap Yellow Display)
* Micro-USB or USB-C cable (for the initial flash)

## 📦 Software Dependencies
You will need the Arduino IDE with the **ESP32 Core** installed (Version 3.x.x supported!). Install the following libraries via the Arduino Library Manager:
* `TFT_eSPI` by Bodmer (Graphics)
* `ArduinoJson` by Benoit Blanchon (API parsing)

## ⚠️ CRITICAL SETUP: TFT_eSPI Configuration
Before compiling, you **MUST** configure the `TFT_eSPI` library to work with the CYD pins, otherwise the screen will be blank or throw a `getTouch` compile error.

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
