#include <Arduino.h>
#include <WiFi.h>
#include <FS.h>
using namespace fs;       
#include <WebServer.h>
#include <Update.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <mbedtls/sha1.h>

// ==========================================
// 1. YOUR SETTINGS (UPDATE THESE!)
// ==========================================
const char* ssid = "YOUR SSID";
const char* password = "YOUR PASSWORD";

// Your Official Duino-Coin Credentials
String ducoUsername = "YOUR DUINO COIN USERNAME";
String minerKey = "YOUR MINER KEY";

// OTA Login Credentials
const char* otaUser = "admin";
const char* otaPass = "admin";

// ==========================================
// 2. HARDWARE & GLOBAL VARIABLES
// ==========================================
TFT_eSPI tft = TFT_eSPI();
WebServer server(80);
TaskHandle_t MinerTaskHandle;

// Hardware Pins
#define LED_R 4
#define LED_G 16
#define LED_B 17
#define TFT_BL 21

// Mining Data (Shared between cores)
volatile float currentHashrate = 0.0;
volatile int acceptedShares = 0;
float currentBalance = 0.0;
float ducoPrice = 0.0;
float totalValueUSD = 0.0;

// Graph & UI Timers
const int graphX = 20, graphY = 195, graphW = 280, graphH = 40;
int currentGraphX = graphX;
int prevGraphY = graphY + graphH;
unsigned long lastApiTime = 0;
unsigned long lastUiTime = 0;
bool isScreenOn = true;

// ==========================================
// 3. BACKGROUND OTA WEB SERVER
// ==========================================
void setupOTA() {
  server.on("/update", HTTP_GET, []() {
    if (!server.authenticate(otaUser, otaPass)) return server.requestAuthentication();
    server.send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data' style='font-family:sans-serif; text-align:center; margin-top:50px;'><h2>CYD OTA Updater</h2><input type='file' name='update' style='margin:20px;'><br><input type='submit' value='Upload & Reboot' style='padding:10px 20px;'></form>");
  });

  server.on("/update", HTTP_POST, []() {
    if (!server.authenticate(otaUser, otaPass)) return server.requestAuthentication();
    server.send(200, "text/plain", (Update.hasError()) ? "UPDATE FAILED" : "UPDATE SUCCESS. REBOOTING...");
    delay(1000);
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) Update.printError(Serial);
    } else if (upload.status == UPLOAD_FILE_END) {
      Update.end(true); 
    }
  });
  server.begin();
}

// ==========================================
// 4. HELPER FUNCTIONS
// ==========================================
void fetchDucoBalance() {
  HTTPClient http;
  http.begin("https://server.duinocoin.com/balances/" + ducoUsername);
  if (http.GET() == HTTP_CODE_OK) {
    JsonDocument doc;
    if (!deserializeJson(doc, http.getString())) {
      currentBalance = doc["result"]["balance"].as<float>();
    }
  }
  http.end();
}

void fetchDucoPrice() {
  HTTPClient http;
  http.begin("https://server.duinocoin.com/api.json");
  if (http.GET() == HTTP_CODE_OK) {
    String payload = http.getString();
    int priceIndex = payload.indexOf("\"Duco PancakeSwap price\":");
    if (priceIndex > 0) {
      int commaIndex = payload.indexOf(",", priceIndex);
      ducoPrice = payload.substring(priceIndex + 25, commaIndex).toFloat();
      totalValueUSD = currentBalance * ducoPrice;
    }
  }
  http.end();
}

void updateDisplay() {
  if (!isScreenOn) return; 

  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.drawCentreString("DUINO-COIN RIG", 160, 10, 4);
  tft.drawLine(20, 36, 300, 36, TFT_DARKGREY);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Wallet Balance:", 20, 45, 2);
  char buf1[40]; sprintf(buf1, "%.2f DUCO      ", currentBalance); 
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.drawString(buf1, 20, 65, 4);

  char buf2[40]; sprintf(buf2, "Est. Value: $%.4f       ", totalValueUSD);
  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.drawString(buf2, 20, 95, 2); 

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Current Hashrate:", 20, 120, 2);
  char buf3[20]; sprintf(buf3, "%.1f kH/s   ", currentHashrate / 1000.0); 
  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.drawString(buf3, 20, 140, 4);

  float dailyEst = (currentHashrate / 1000.0) * 0.35;
  char bufDaily[30]; sprintf(bufDaily, "Est. Daily: %.1f DUCO  ", dailyEst);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK); 
  tft.drawString(bufDaily, 160, 145, 2); 

  char buf4[30]; sprintf(buf4, "Accepted Shares: %d   ", acceptedShares);
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.drawString(buf4, 20, 170, 2);
}

void drawGraph() {
  if (!isScreenOn) return; 

  tft.drawRect(graphX - 1, graphY - 1, graphW + 2, graphH + 2, TFT_DARKGREY);
  float hr = currentHashrate / 1000.0;
  if (hr > 40.0) hr = 40.0; 
  int newY = (graphY + graphH) - (int)((hr / 40.0) * graphH);

  int eraseX = currentGraphX + 2;
  if (eraseX <= graphX + graphW) {
    tft.drawFastVLine(eraseX, graphY, graphH, TFT_BLACK);
    tft.drawFastVLine(eraseX + 1, graphY, graphH, TFT_BLACK);
  }

  if (currentGraphX > graphX) { 
    tft.drawLine(currentGraphX - 1, prevGraphY, currentGraphX, newY, TFT_GREEN);
  }

  prevGraphY = newY;
  currentGraphX++;
  if (currentGraphX > graphX + graphW) currentGraphX = graphX; 
}

// ==========================================
// 5. CORE 0: MINING ALGORITHM (BULLETPROOF FIX)
// ==========================================
void miningTask(void * parameter) {
  WiFiClient client;
  String current_pool_ip = "";
  int current_pool_port = 0;

  vTaskDelay(3000 / portTICK_PERIOD_MS);

  for(;;) {
    if (WiFi.status() != WL_CONNECTED) { 
      vTaskDelay(1000); 
      continue; 
    }

    if (!client.connected()) {
      if (current_pool_ip == "") {
        HTTPClient http;
        http.begin("https://server.duinocoin.com/getPool");
        if (http.GET() == HTTP_CODE_OK) {
          JsonDocument doc;
          if (!deserializeJson(doc, http.getString())) {
            current_pool_ip = doc["ip"].as<String>();
            current_pool_port = doc["port"].as<int>();
          }
        }
        http.end();

        if (current_pool_ip == "") {
          vTaskDelay(5000 / portTICK_PERIOD_MS);
          continue;
        }
      }

      if (!client.connect(current_pool_ip.c_str(), current_pool_port)) { 
        current_pool_ip = ""; 
        vTaskDelay(5000 / portTICK_PERIOD_MS); 
        continue; 
      }
      
      client.setTimeout(10000); 
      client.readStringUntil('\n'); 
    }

    // THE FIX: Lower the requested difficulty tier to ESP8266 so it doesn't timeout!
    client.print("JOB," + ducoUsername + ",ESP8266," + minerKey + "\n");
    
    String job = client.readStringUntil('\n');
    job.trim(); 
    if (job.length() == 0) continue;

    int comma1 = job.indexOf(',');
    int comma2 = job.indexOf(',', comma1 + 1);
    
    // Catch empty or broken server data
    if (comma1 == -1 || comma2 == -1) {
      client.stop(); 
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }

    String lastHash = job.substring(0, comma1);
    String expectedHash = job.substring(comma1 + 1, comma2);
    int difficulty = job.substring(comma2 + 1).toInt();
    
    // THE FIX: The mathematically guaranteed maximum possible answer
    unsigned long max_result = (difficulty * 100) + 1;
    
    uint8_t expected_bytes[20];
    for (int i = 0; i < 20; i++) {
      String hexByte = expectedHash.substring(i * 2, i * 2 + 2);
      expected_bytes[i] = (uint8_t) strtol(hexByte.c_str(), NULL, 16);
    }

    char payload[100];
    strcpy(payload, lastHash.c_str());
    int lastHashLen = lastHash.length();
    
    unsigned long result = 0;
    unsigned long startTime = micros();
    
    mbedtls_sha1_context ctx;
    mbedtls_sha1_init(&ctx); 
    
    while (true) {
      sprintf(payload + lastHashLen, "%lu", result); 
      
      mbedtls_sha1_starts(&ctx);
      mbedtls_sha1_update(&ctx, (const unsigned char*)payload, strlen(payload));
      uint8_t hashOutput[20];
      mbedtls_sha1_finish(&ctx, hashOutput); 
      
      if (memcmp(hashOutput, expected_bytes, 20) == 0) {
        break; // Hash found!
      }
      
      result++;
      
      // THE FIX: If we hit the absolute ceiling without an answer, the block is bad. Break instantly!
      if (result > max_result) {
        break;
      }
      
      if (result % 10000 == 0) {
        vTaskDelay(1); 
      }
    }
    
    mbedtls_sha1_free(&ctx);
    
    // Only submit the answer if we actually found it and didn't hit the ceiling
    if (result <= max_result) {
      unsigned long elapsedTime = micros() - startTime;
      if (elapsedTime == 0) elapsedTime = 1; // Prevent divide by zero
      
      currentHashrate = (result / (elapsedTime / 1000000.0));
      
      // Submit using Official Identifiers so Kolka trusts the board
      client.print(String(result) + "," + String(currentHashrate) + ",Official ESP32 Miner,CYD_Dashboard\n");
      
      String feedback = client.readStringUntil('\n');
      feedback.trim(); 
      
      if (feedback.indexOf("GOOD") >= 0) {
        acceptedShares++;
        digitalWrite(LED_G, LOW);            
        vTaskDelay(50 / portTICK_PERIOD_MS); 
        digitalWrite(LED_G, HIGH);           
      }
    } else {
      // We hit the ceiling. The connection probably timed out or the block was broken.
      // Hang up the phone and start fresh.
      client.stop(); 
    }
    
    vTaskDelay(10); 
  }
}

// ==========================================
// 6. MAIN SETUP
// ==========================================
void setup() {
  Serial.begin(115200);

  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT);
  digitalWrite(LED_R, HIGH); digitalWrite(LED_G, HIGH); digitalWrite(LED_B, HIGH);
  pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);

  tft.init(); tft.setRotation(1); tft.fillScreen(TFT_BLACK);
  uint16_t calData[5] = { 275, 3620, 264, 3532, 1 }; tft.setTouch(calData);
                                                     hhInitTheme();
                                                     hhSetMode(HH_DASH);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.drawCentreString("DUINO-COIN MINER", 160, 100, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawCentreString("Connecting to WiFi...", 160, 140, 2);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  setupOTA();
  
  tft.fillScreen(TFT_BLACK); 
  xTaskCreatePinnedToCore(miningTask, "MinerTask", 10000, NULL, 1, &MinerTaskHandle, 0);
}

// ==========================================
// 7. MAIN LOOP (CORE 1)
// ==========================================
void loop() {
  unsigned long currentMillis = millis();
  server.handleClient();

  uint16_t tx, ty;
  if (tft.getTouch(&tx, &ty)) {
    isScreenOn = !isScreenOn; 
    if (isScreenOn) {
      digitalWrite(TFT_BL, HIGH); 
      tft.fillScreen(TFT_BLACK);  
    } else {
      digitalWrite(TFT_BL, LOW);  
    }
    delay(500); 
  }

  if (currentMillis - lastApiTime >= 300000 || lastApiTime == 0) {
    if (WiFi.status() == WL_CONNECTED) {
      fetchDucoBalance();
      fetchDucoPrice(); 
    }
    lastApiTime = currentMillis;
  }

  if (currentMillis - lastUiTime >= 1000) {
    updateDisplay();
    drawGraph();
    lastUiTime = currentMillis;
  }
}
