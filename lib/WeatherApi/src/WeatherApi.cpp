#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "WeatherApi.h"

static ApiError httpGet(const String& url, String& body) {
  ApiError err;
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  if (!http.begin(url)) {
    err.ok = false;
    err.httpCode = -1;
    err.message = F("http.begin failed");
    return err;
  }
  int code = http.GET();
  if (code <= 0) {
    err.ok = false;
    err.httpCode = code;
    err.message = F("GET failed");
    http.end();
    return err;
  }
  body = http.getString();
  http.end();
  err.ok = (code == HTTP_CODE_OK);
  err.httpCode = code;
  if (!err.ok) err.message = F("Non-200 status");
  return err;
}

ApiError WeatherApi::getCurrent(WeatherData& out) {
  String url = String("http://api.openweathermap.org/data/2.5/weather?lat=") + OWM_LAT +
               "&lon=" + OWM_LON + "&units=" + OWM_UNITS + "&lang=" + OWM_LANG +
               "&appid=" + OWM_API_KEY;

  String body;
  ApiError httpErr = httpGet(url, body);
  if (!httpErr.ok) return httpErr;

  DynamicJsonDocument doc(4096);
  auto jerr = deserializeJson(doc, body);
  if (jerr) {
    ApiError e; e.ok = false; e.httpCode = 200; e.message = String("JSON error: ") + jerr.c_str();
    return e;
  }

  JsonVariant Jmain = doc["main"];
  JsonVariant Jweather0 = doc["weather"][0];

  out.temp        = Jmain["temp"].as<float>();
  out.humidity    = Jmain["humidity"].as<float>();
  out.icon        = Jweather0["icon"].as<const char*>() ? Jweather0["icon"].as<const char*>() : "";
  out.description = Jweather0["description"].as<const char*>() ? Jweather0["description"].as<const char*>() : "";
  out.fetchedAtMs = millis();

  ApiError ok; ok.ok = true; ok.httpCode = 200; return ok;
}

const char* WeatherApi::mapIconToAsset(const String& iconCode) {
  // Bạn có thể map về tên bitmap 16x16 nội bộ để draw.
  // 01d/01n: clear, 02d: few clouds, 09/10: rain, 11: thunder, 13: snow, 50: mist
  if (iconCode.startsWith("01")) return "weather_clear";
  if (iconCode.startsWith("02")) return "weather_few";
  if (iconCode.startsWith("03") || iconCode.startsWith("04")) return "weather_cloudy";
  if (iconCode.startsWith("09") || iconCode.startsWith("10")) return "weather_rain";
  if (iconCode.startsWith("11")) return "weather_storm";
  if (iconCode.startsWith("13")) return "weather_snow";
  if (iconCode.startsWith("50")) return "weather_mist";
  return "weather_na";
}
