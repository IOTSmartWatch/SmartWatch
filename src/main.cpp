#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>             // đổi sang SH1106
#define SSD1306_WHITE SH110X_WHITE       // ánh xạ màu để giữ nguyên code cũ
#define SSD1306_BLACK SH110X_BLACK
#include <TimeLib.h>

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);  // đổi class

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
bool wifiOn         = true;

// ---------- prototypes icon draw functions ----------
void drawClockIcon(int x, int y);
void drawSunIcon(int x, int y);
void drawCalendarIcon(int x, int y);
void drawYouTubeIcon(int x, int y);
void drawBTCIcon(int x, int y);
void drawNewsIcon(int x, int y);
void drawWifiIcon(int x, int y, uint16_t color = SSD1306_WHITE);

// ---------- header & status ----------
void drawStatusBar() {
  int bx = 96, by = 2, bw = 24, bh = 10;
  display.drawRect(bx, by, bw-4, bh, SSD1306_WHITE);
  display.fillRect(bx + bw - 4, by + 3, 3, 4, SSD1306_WHITE);
  int fillw = map(batteryPercent, 0, 100, 0, (bw-6)-2);
  if (fillw < 0) fillw = 0;
  display.fillRect(bx+2, by+2, fillw, bh-4, SSD1306_WHITE);
  if (wifiOn) drawWifiIcon(bx-18, 0, SSD1306_WHITE);
}

void drawHeader(const char* title, void (*drawIconFn)(int,int)) {
  const int HEADER_W = 72;
  display.fillRect(0, 0, HEADER_W, 18, SSD1306_WHITE);
  if (drawIconFn) drawIconFn(2, 1);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(22, 4);
  display.println(title);
  display.setTextColor(SSD1306_WHITE);
  drawStatusBar();
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
  display.setCursor(8, 35); display.print("28"); display.write((char)247); display.print("C ");
  display.setTextSize(1);   display.print("Sunny");
  display.setCursor(8, 50); display.print("Hum 68% TP HCM");
}

void drawEvents() {
  drawHeader("Events", drawCalendarIcon);
  display.setTextSize(1);
  display.setCursor(6, 26); display.print("Meeting     15:00");
  display.setCursor(6, 38); display.print("Deadline    21:00");
  display.setCursor(6, 50); display.print("Gym         19:00");
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
  display.drawCircle(x+8, y+8, 7, SSD1306_BLACK);
  display.fillCircle (x+8, y+8, 1, SSD1306_BLACK);
  display.drawLine(x+8, y+8, x+8,  y+3, SSD1306_BLACK);
  display.drawLine(x+8, y+8, x+12, y+8, SSD1306_BLACK);
}

void drawSunIcon(int x, int y) {
  int cx = x+8, cy = y+8;
  display.drawCircle(cx, cy, 5, SSD1306_BLACK);
  display.drawLine(cx, cy-7, cx, cy-3, SSD1306_BLACK);
  display.drawLine(cx, cy+3, cx, cy+7, SSD1306_BLACK);
  display.drawLine(cx-7, cy, cx-3, cy, SSD1306_BLACK);
  display.drawLine(cx+3, cy, cx+7, cy, SSD1306_BLACK);
  display.drawLine(cx-5, cy-5, cx-3, cy-3, SSD1306_BLACK);
  display.drawLine(cx+5, cy-5, cx+3, cy-3, SSD1306_BLACK);
  display.drawLine(cx-5, cy+5, cx-3, cy+3, SSD1306_BLACK);
  display.drawLine(cx+5, cy+5, cx+3, cy+3, SSD1306_BLACK);
}

void drawCalendarIcon(int x, int y) {
  display.drawRoundRect(x+1, y+2, 14, 12, 2, SSD1306_BLACK);
  display.fillRect     (x+2, y+3, 12, 3, SSD1306_BLACK);
  display.drawFastVLine(x+4,  y+1, 3, SSD1306_BLACK);
  display.drawFastVLine(x+11, y+1, 3, SSD1306_BLACK);
  display.drawFastHLine(x+3,  y+9, 10, SSD1306_BLACK);
  display.drawFastVLine(x+8,  y+7, 6,  SSD1306_BLACK);
}

void drawYouTubeIcon(int x, int y) {
  display.drawRoundRect(x+1, y+3, 14, 10, 2, SSD1306_BLACK);
  int cx = x + 8, cy = y + 8;
  display.fillTriangle(cx - 3, cy - 3, cx - 3, cy + 3, cx + 4, cy, SSD1306_BLACK);
}

void drawBTCIcon(int x, int y) {
  int cx = x+8, cy = y+8;
  display.drawCircle(cx, cy, 7, SSD1306_BLACK);
  display.drawFastVLine(cx-1, y+3, 10, SSD1306_BLACK);
  display.drawFastVLine(cx,   y+3, 10, SSD1306_BLACK);
  display.drawFastHLine(cx-1, y+4, 6, SSD1306_BLACK);
  display.drawFastVLine(cx+4, y+4, 3, SSD1306_BLACK);
  display.drawFastHLine(cx-1, y+8, 6, SSD1306_BLACK);
  display.drawFastVLine(cx+4, y+8, 3, SSD1306_BLACK);
  display.drawFastHLine(cx-1, y+12, 6, SSD1306_BLACK);
}

void drawNewsIcon(int x, int y) {
  display.drawRoundRect(x+1, y+2, 14, 12, 2, SSD1306_BLACK);
  display.fillRect     (x+2, y+3,  8,  3,  SSD1306_BLACK);
  display.drawRect     (x+2, y+7,  5,  5,  SSD1306_BLACK);
  display.drawFastHLine(x+8, y+8,  6,  SSD1306_BLACK);
  display.drawFastHLine(x+8, y+11, 6,  SSD1306_BLACK);
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

// ================= setup / loop =================
void setup() {
  Serial.begin(115200);
  if (BTN_ACTIVE_LOW) pinMode(BUTTON_PIN, INPUT_PULLUP);
  else                pinMode(BUTTON_PIN, INPUT);

  Wire.begin(21, 22);

  // SH1106 begin:
  if (!display.begin(0x3C, true)) {      // nếu không hiện, thử 0x3D
    Serial.println(F("SH1106 allocation failed"));
    for(;;);
  }
  // Optional: tinh chỉnh tương phản nếu cần
  // display.setContrast(0x80);

  display.clearDisplay();
  display.display();

  setTime(12,0,0,1,1,2025);
  renderScreenOnce();
}

void loop() {
  handleButton();
}
