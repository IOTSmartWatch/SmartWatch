#include "api_manager.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

bool fetch_api(const char* url, ApiData &data) {
  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  if (code != 200) { http.end(); return false; }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<2048> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) return false;

  data.subs = doc["youtube"] | 0;
  data.btc  = doc["btc"] | 0.0;
  data.eth  = doc["eth"] | 0.0;

  // rss: array of objects with "title"
  for (int i = 0; i < 3; i++) {
    data.news[i] = "";
    if (doc["rss"][i].is<JsonObject>()) {
      data.news[i] = (const char*)(doc["rss"][i]["title"] | "");
    }
  }
  return true;
}
