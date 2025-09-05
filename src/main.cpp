#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <TimeLib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#include "api_types.h"
#include "api_config.h"
#include "secrets.h"
#include "WeatherApi.h"
#include "GCalApi.h"

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
enum ScreenID {SCR_CLOCK, SCR_WEATHER, SCR_EVENTS, SCR_YOUTUBE, SCR_CRYPTO, SCR_NEWS, SCR__COUNT};
ScreenID currentScreen = SCR_CLOCK;

// Debounce state
bool btnStable   = (BTN_ACTIVE_LOW ? HIGH : LOW);
int  btnRawPrev  = (BTN_ACTIVE_LOW ? HIGH : LOW);
unsigned long btnLastEdgeMs = 0;
unsigned long btnPressStart = 0;

// Status bar demo
int  batteryPercent = 80;
bool wifiOn         = false;

// Data state
static WeatherData gWeather;
static std::vector<CalendarEvent> gEvents;

static unsigned long tNextWeather = 0;
static unsigned long tNextGCal    = 0;

// ---------- prototypes icon draw functions ----------
void drawClockIcon(int x, int y);
void drawSunIcon(int x, int y);
void drawCalendarIcon(int x, int y);
void drawYouTubeIcon(int x, int y);
void drawBTCIcon(int x, int y);
void drawNewsIcon(int x, int y);
void drawWifiIcon(int x, int y, uint16_t color = SH110X_WHITE);

// ---------- header & status ----------
void drawStatusBar() {
  int bx = 96, by = 2, bw = 24, bh = 10;
  display.drawRect(bx, by, bw-4, bh, SH110X_WHITE);
  display.fillRect(bx + bw - 4, by + 3, 3, 4, SH110X_WHITE);
  int fillw = map(batteryPercent, 0, 100, 0, (bw-6)-2);
  if (fillw < 0) fillw = 0;
  display.fillRect(bx+2, by+2, fillw, bh-4, SH110X_WHITE);
  if (wifiOn) drawWifiIcon(bx-18, 0, SH110X_WHITE);
}

void drawHeader(const char* title, void (*drawIconFn)(int,int)) {
  const int HEADER_W = 72;
  display.fillRect(0, 0, HEADER_W, 18, SH110X_WHITE);
  if (drawIconFn) drawIconFn(2, 1);
  display.setTextColor(SH110X_BLACK);
  display.setTextSize(1);
  display.setCursor(22, 4);
  display.println(title);
  display.setTextColor(SH110X_WHITE);
  drawStatusBar();
}

// ---------- helpers ----------
static void fmtHHMMfromEpoch(time_t epoch, char* out, size_t n) {
  struct tm t; localtime_r(&epoch, &t);
  snprintf(out, n, "%02d:%02d", t.tm_hour, t.tm_min);
}

// ---------- screens ----------
void drawClock() {
  drawHeader("Clock", drawClockIcon);
  char buf[16];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hour(), minute(), second());
  int16_t x1,y1; uint16_t w,h;
  display.setTextSize(2);
  display.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, 34);
  display.print(buf);
}

void drawWeather() {
  drawHeader("Weather", drawSunIcon);
  display.setTextSize(2);
  display.setCursor(8, 35);
  display.cp437(true);
  if (isnan(gWeather.temp)) {
    display.print("--");
  } else {
    display.print((int)round(gWeather.temp));
  }
  display.write((char)248); // degree symbol
  display.print(OWM_UNITS[0]=='m' ? "C " : "F ");
  display.setTextSize(1);
  display.print(gWeather.description.length()? gWeather.description : "n/a");
  display.setCursor(8, 50);
  if (isnan(gWeather.humidity)) display.print("Hum --%");
  else { display.print("Hum "); display.print((int)round(gWeather.humidity)); display.print("%"); }
}

void drawEvents() {
  drawHeader("Events", drawCalendarIcon);
  display.setTextSize(1);
  int y = 26;
  int shown = 0;
  for (auto &ev : gEvents) {
    char hhmm[8]; fmtHHMMfromEpoch(ev.startUtc, hhmm, sizeof(hhmm));
    display.setCursor(6, y);
    if (ev.allDay) {
      display.print(ev.title.substring(0, 12)); display.print("  All-day");
    } else {
      display.print(ev.title.substring(0, 12)); display.print("  "); display.print(hhmm);
    }
    y += 12;
    if (++shown >= 3) break;
  }
  if (shown == 0) {
    display.setCursor(6, y); display.print("No events");
  }
}

void drawYouTube() {
  drawHeader("YouTube", drawYouTubeIcon);
  display.setTextSize(2);
  display.setCursor(8, 32); display.print("Subs: 123k");
}

void drawCrypto() {
  drawHeader("Crypto", drawBTCIcon);
  display.setTextSize(2);
  display.setCursor(8, 32); display.print("BTC 64k");
  display.setTextSize(1);
  display.setCursor(8, 50); display.print("ETH 3.1k   BTC/ETH 20.6");
}

void drawNews() {
  drawHeader("News", drawNewsIcon);
  display.setTextSize(1);
  display.setCursor(4, 28); display.print("- AI trend 2025");
  display.setCursor(4, 40); display.print("- SpaceX launch");
  display.setCursor(4, 52); display.print("- Local showers");
}

void renderScreenOnce() {
  display.clearDisplay();
  switch (currentScreen) {
    case SCR_CLOCK:   drawClock();   break;
    case SCR_WEATHER: drawWeather(); break;
    case SCR_EVENTS:  drawEvents();  break;
    case SCR_YOUTUBE: drawYouTube(); break;
    case SCR_CRYPTO:  drawCrypto();  break;
    case SCR_NEWS:    drawNews();    break;
  }
  display.display();
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
      if (btnStable) { btnPressStart = now; }
      else {
        unsigned long held = now - btnPressStart;
        currentScreen = (ScreenID)((currentScreen + (held >= LONG_PRESS_MS ? -1 : 1) + SCR__COUNT) % SCR__COUNT);
        renderScreenOnce();
      }
    }
  }
}

// ================= ICONS (16x16) =================
void drawClockIcon(int x, int y) {
  display.drawCircle(x+8, y+8, 7, SH110X_BLACK);
  display.fillCircle (x+8, y+8, 1, SH110X_BLACK);
  display.drawLine(x+8, y+8, x+8,  y+3, SH110X_BLACK);
  display.drawLine(x+8, y+8, x+12, y+8, SH110X_BLACK);
}

void drawSunIcon(int x, int y) {
  int cx = x+8, cy = y+8;
  display.drawCircle(cx, cy, 5, SH110X_BLACK);
  display.drawLine(cx, cy-7, cx, cy-3, SH110X_BLACK);
  display.drawLine(cx, cy+3, cx, cy+7, SH110X_BLACK);
  display.drawLine(cx-7, cy, cx-3, cy, SH110X_BLACK);
  display.drawLine(cx+3, cy, cx+7, cy, SH110X_BLACK);
  display.drawLine(cx-5, cy-5, cx-3, cy-3, SH110X_BLACK);
  display.drawLine(cx+5, cy-5, cx+3, cy-3, SH110X_BLACK);
  display.drawLine(cx-5, cy+5, cx-3, cy+3, SH110X_BLACK);
  display.drawLine(cx+5, cy+5, cx+3, cy+3, SH110X_BLACK);
}

void drawCalendarIcon(int x, int y) {
  display.drawRoundRect(x+1, y+2, 14, 12, 2, SH110X_BLACK);
  display.fillRect     (x+2, y+3, 12, 3, SH110X_BLACK);
  display.drawFastVLine(x+4,  y+1, 3, SH110X_BLACK);
  display.drawFastVLine(x+11, y+1, 3, SH110X_BLACK);
  display.drawFastHLine(x+3,  y+9, 10, SH110X_BLACK);
  display.drawFastVLine(x+8,  y+7, 6,  SH110X_BLACK);
}

void drawYouTubeIcon(int x, int y) {
  display.drawRoundRect(x+1, y+3, 14, 10, 2, SH110X_BLACK);
  int cx = x + 8, cy = y + 8;
  display.fillTriangle(cx - 3, cy - 3, cx - 3, cy + 3, cx + 4, cy, SH110X_BLACK);
}

void drawBTCIcon(int x, int y) {
  int cx = x+8, cy = y+8;
  display.drawCircle(cx, cy, 7, SH110X_BLACK);
  display.drawFastVLine(cx-1, y+3, 10, SH110X_BLACK);
  display.drawFastVLine(cx,   y+3, 10, SH110X_BLACK);
  display.drawFastHLine(cx-1, y+4, 6, SH110X_BLACK);
  display.drawFastVLine(cx+4, y+4, 3, SH110X_BLACK);
  display.drawFastHLine(cx-1, y+8, 6, SH110X_BLACK);
  display.drawFastVLine(cx+4, y+8, 3, SH110X_BLACK);
  display.drawFastHLine(cx-1, y+12, 6, SH110X_BLACK);
}

void drawNewsIcon(int x, int y) {
  display.drawRoundRect(x+1, y+2, 14, 12, 2, SH110X_BLACK);
  display.fillRect     (x+2, y+3,  8,  3,  SH110X_BLACK);
  display.drawRect     (x+2, y+7,  5,  5,  SH110X_BLACK);
  display.drawFastHLine(x+8, y+8,  6,  SH110X_BLACK);
  display.drawFastHLine(x+8, y+11, 6,  SH110X_BLACK);
}

void drawWifiIcon(int x, int y, uint16_t color) {
  int cx = x + 8;
  int cy = y + 12;
  display.drawCircleHelper(cx, cy, 3, 0x1, color);
  display.drawCircleHelper(cx, cy, 3, 0x2, color);
  display.drawCircleHelper(cx, cy, 5, 0x1, color);
  display.drawCircleHelper(cx, cy, 5, 0x2, color);
  display.drawCircleHelper(cx, cy, 7, 0x1, color);
  display.drawCircleHelper(cx, cy, 7, 0x2, color);
  display.fillCircle(cx, cy, 2, color);
}

// ================= networking & updates =================
static void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(100);
  }
  wifiOn = (WiFi.status() == WL_CONNECTED);
}

static void maybeUpdateWeather() {
  unsigned long now = millis();
  if (now < tNextWeather) return;
  ApiError e = WeatherApi::getCurrent(gWeather);
  if (!e.ok) {
    Serial.printf("[Weather] err http=%d %s\n", e.httpCode, e.message.c_str());
    tNextWeather = now + 5000; // retry sooner
  } else {
    Serial.printf("[Weather] ok: T=%.1f H=%.0f icon=%s\n", gWeather.temp, gWeather.humidity, gWeather.icon.c_str());
    tNextWeather = now + WEATHER_REFRESH_MS;
    if (currentScreen == SCR_WEATHER) renderScreenOnce();
  }
}

static void maybeUpdateGCal() {
  unsigned long now = millis();
  if (now < tNextGCal) return;
  std::vector<CalendarEvent> tmp;
  ApiError e = GCalApi::getUpcoming(tmp, 5);
  if (!e.ok) {
    Serial.printf("[GCal] err http=%d %s\n", e.httpCode, e.message.c_str());
    tNextGCal = now + 10000; // retry
  } else {
    gEvents.swap(tmp);
    Serial.printf("[GCal] ok: %u events\n", (unsigned)gEvents.size());
    tNextGCal = now + GCAL_REFRESH_MS;
    if (currentScreen == SCR_EVENTS) renderScreenOnce();
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

  // Thời gian demo cho Clock (nếu chưa dùng NTP)
  setTime(12,0,0,1,1,2025);

  connectWiFi();
  renderScreenOnce();
}

void loop() {
  handleButton();
  if (wifiOn) {
    maybeUpdateWeather();
    maybeUpdateGCal();
  } else {
    static unsigned long t = 0; 
    if (millis() - t > 5000) { connectWiFi(); t = millis(); }
  }
}
