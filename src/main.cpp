#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <time.h>
#include <TimeLib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= BUTTON =================
#define BUTTON_PIN       4
#define BTN_ACTIVE_LOW   1
#define DEBOUNCE_MS      35
#define LONG_PRESS_MS    500

// ================= UI =================
enum ScreenID {SCR_CLOCK, SCR_SCHEDULER, SCR__COUNT};
ScreenID currentScreen = SCR_CLOCK;

bool btnStable   = (BTN_ACTIVE_LOW ? HIGH : LOW);
int  btnRawPrev  = (BTN_ACTIVE_LOW ? HIGH : LOW);
unsigned long btnLastEdgeMs = 0;
unsigned long btnPressStart = 0;

// ================= WiFi & NTP =================
const char* ssid     = "Your_SSID";
const char* password = "Your_PASSWORD";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec     = 7 * 3600;   // GMT+7 VN
const int   daylightOffset_sec = 0;

// ================= Scheduler =================
unsigned long lastSecond = 0;
unsigned long lastMinute = 0;
unsigned long lastSync   = 0;

// ---------- helpers ----------
void syncSystemTime() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    setTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
            timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
  }
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.printf("Connecting to WiFi: %s\n", ssid);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(200);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? "\nWiFi connected!" : "\nWiFi failed!");
}

// ---------- UI screens ----------
void drawClock() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10, 20);
  display.printf("%02d:%02d:%02d", hour(), minute(), second());
  display.display();
}

void drawSchedulerStatus() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("Scheduler running:");
  display.setCursor(0, 25);
  display.printf("Last sync: %lus ago", (millis() - lastSync)/1000);
  display.setCursor(0, 40);
  display.printf("WiFi: %s", WiFi.isConnected() ? "OK" : "Disconnected");
  display.display();
}

void renderScreenOnce() {
  switch (currentScreen) {
    case SCR_CLOCK:      drawClock(); break;
    case SCR_SCHEDULER:  drawSchedulerStatus(); break;
  }
}

// ---------- button ----------
void handleButton() {
  unsigned long now = millis();
  int rawRead = digitalRead(BUTTON_PIN);
  bool pressed = BTN_ACTIVE_LOW ? (rawRead == LOW) : (rawRead == HIGH);
  if (rawRead != btnRawPrev) { btnRawPrev = rawRead; btnLastEdgeMs = now; }
  if ((now - btnLastEdgeMs) > DEBOUNCE_MS) {
    bool stableNow = pressed;
    if (stableNow != btnStable) {
      btnStable = stableNow;
      if (!btnStable) {
        unsigned long held = now - btnPressStart;
        currentScreen = (ScreenID)((currentScreen + (held >= LONG_PRESS_MS ? -1 : 1) + SCR__COUNT) % SCR__COUNT);
        renderScreenOnce();
      } else {
        btnPressStart = now;
      }
    }
  }
}

// ================= setup / loop =================
void setup() {
  Serial.begin(115200);
  if (BTN_ACTIVE_LOW) pinMode(BUTTON_PIN, INPUT_PULLUP);
  else                pinMode(BUTTON_PIN, INPUT);

  Wire.begin(21, 22);

  if (!display.begin(0x3C, true)) {
    Serial.println(F("SH1106 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.display();

  connectWiFi();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  syncSystemTime();
  renderScreenOnce();
}

void loop() {
  handleButton();
  unsigned long now = millis();

  // 1s: cập nhật đồng hồ
  if (now - lastSecond >= 1000) {
    lastSecond = now;
    syncSystemTime();
    if (currentScreen == SCR_CLOCK) renderScreenOnce();
  }

  // 1p: tác vụ định kỳ
  if (now - lastMinute >= 60000) {
    lastMinute = now;
    Serial.println("[Scheduler] 1-minute task running...");
    if (currentScreen == SCR_SCHEDULER) renderScreenOnce();
  }

  // 10p: resync NTP
  if (now - lastSync >= 600000) {
    lastSync = now;
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("[Scheduler] NTP resynced");
  }
}
