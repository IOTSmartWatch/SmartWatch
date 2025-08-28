#include "display_manager.h"
#include <Wire.h>

// ====== chọn 1 driver: SSD1306 (phổ biến) ======
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void display_init(bool /*isSH1106*/) {
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setTextWrap(false);
  display.display();
}

void draw_lines(const String& a, const String& b="", const String& c="") {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);  display.println(a);
  display.setCursor(0,12); display.println(b);
  display.setCursor(0,24); display.println(c);
  display.display();
}

void display_show_status(const String& l1, const String& l2, const String& l3) {
  draw_lines(l1, l2, l3);
}

void display_show_subs(long subs) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0,0);  display.println("YT Subs");
  display.setTextSize(2);
  display.setCursor(0,24); display.println(subs);
  display.display();
}

void display_show_crypto(float btc, float eth) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);  display.println("BTC (USD):");
  display.setCursor(0,12); display.println(String(btc, 2));
  display.setCursor(0,28); display.println("ETH (USD):");
  display.setCursor(0,40); display.println(String(eth, 2));
  display.display();
}

static String trim_to_width(const String& s, uint8_t maxChars) {
  if (s.length() <= maxChars) return s;
  return s.substring(0, maxChars-3) + "...";
}

void display_show_news(String news[3]) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);  display.println("News (Top 3):");
  for (int i=0;i<3;i++) {
    display.setCursor(0, 12*(i+1));
    display.println(trim_to_width(news[i], 21)); // cắt để vừa màn hình
  }
  display.display();
}
