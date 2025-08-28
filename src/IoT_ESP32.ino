#include <Arduino.h>
#include "wifi_manager.h"
#include "time_manager.h"
#include "display_manager.h"
#include "api_manager.h"
#include "thingspeak_manager.h"

// ===== User Config =====
const char* WIFI_SSID     = "Phong Tro 2";
const char* WIFI_PASSWORD = "0334169382";
const char* SERVER_URL    = "http://192.168.181.128:8000/iot/data";
const char* TS_WRITE_KEY  = "B6OFVT1RJGCQGXEA";

const bool OLED_IS_SH1106 = false;

// Button
#define BUTTON_PIN 27
int page = 0;
unsigned long lastPressMs = 0;

// Intervals
const uint32_t TS_INTERVAL_MS = 60000;
unsigned long lastTsSend = 0;
const uint32_t API_INTERVAL_MS = 10000;
unsigned long lastApiFetch = 0;

ApiData g_data = {0, 0, 0, {"", "", ""}};

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  display_init(OLED_IS_SH1106);
  display_show_status("Booting...", "", "");

  wifi_connect(WIFI_SSID, WIFI_PASSWORD);
  if (WiFi.status() != WL_CONNECTED) {
    display_show_status("WiFi failed!", "Check SSID/PASS", "");
  } else {
    display_show_status("WiFi OK", WiFi.localIP().toString(), "");
  }

  time_init(); // optional NTP
  delay(300);
}

void loop() {
  // Button (debounce)
  if (digitalRead(BUTTON_PIN) == LOW && (millis() - lastPressMs) > 300) {
    page = (page + 1) % 3;  // 0=subs, 1=crypto, 2=news
    lastPressMs = millis();
  }

  // Fetch API
  if (millis() - lastApiFetch > API_INTERVAL_MS) {
    lastApiFetch = millis();
    if (WiFi.status() == WL_CONNECTED) {
      ApiData tmp;
      if (fetch_api(SERVER_URL, tmp)) {
        g_data = tmp;
      } else {
        Serial.println("Fetch API failed");
      }
    }
  }

  // Display
  if (page == 0) {
    display_show_subs(g_data.subs);
  } else if (page == 1) {
    display_show_crypto(g_data.btc, g_data.eth);
  } else {
    display_show_news(g_data.news); // 3 titles
  }

  // ThingSpeak update
  if (millis() - lastTsSend > TS_INTERVAL_MS) {
    lastTsSend = millis();
    if (WiFi.status() == WL_CONNECTED) {
      bool ok = ts_send_update(TS_WRITE_KEY, g_data.subs, g_data.btc, g_data.eth, g_data.news[0]);
      Serial.println(ok ? "ThingSpeak: OK" : "ThingSpeak: FAIL");
    }
  }

  delay(30);
}
