#include "wifi_manager.h"

void wifi_connect(const char* ssid, const char* password) {
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  uint8_t tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi OK: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi FAILED");
  }
}
